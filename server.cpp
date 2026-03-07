#define BLIND_SIG_IMPLEMENTATION
#include "core/core.h"
#include "httplib.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

struct Poll {
    std::string id;
    std::string title;
    std::string public_key;
    std::string secret_key;
    std::string created_at;
    bool is_open = true;
    std::vector<std::string> candidates;
    std::unordered_map<std::string, int> votes;
    std::unordered_set<std::string> issued_tokens;
    std::unordered_set<std::string> consumed_tokens;
    std::unordered_set<std::string> used_ballot_ids;
};

std::unordered_map<std::string, Poll> g_polls;
std::mutex g_polls_mutex;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string JsonEscape(const std::string &input) {
    std::string escaped;
    escaped.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

std::string JsonUnescape(const std::string &input) {
    std::string out;
    out.reserve(input.size());
    bool escaping = false;
    for (char c : input) {
        if (!escaping) {
            if (c == '\\') {
                escaping = true;
            } else {
                out += c;
            }
            continue;
        }

        switch (c) {
            case '\\': out += '\\'; break;
            case '"': out += '"'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            default: out += c; break;
        }
        escaping = false;
    }
    return out;
}

std::string Quote(const std::string &value) {
    return "\"" + JsonEscape(value) + "\"";
}

std::string CurrentTimestampUtc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string RandomHex(size_t bytes) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 15);

    std::string out;
    out.reserve(bytes * 2);
    constexpr char kHex[] = "0123456789abcdef";
    for (size_t i = 0; i < bytes * 2; ++i) {
        out += kHex[dist(rng)];
    }
    return out;
}

std::string GeneratePollId() {
    return "poll_" + RandomHex(8);
}

std::string GenerateToken() {
    return "tok_" + RandomHex(16);
}

int CountTotalVotes(const Poll &poll) {
    int total = 0;
    for (const auto &candidate : poll.candidates) {
        auto it = poll.votes.find(candidate);
        if (it != poll.votes.end()) {
            total += it->second;
        }
    }
    return total;
}

std::string BuildResultsArrayJson(const Poll &poll) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto &candidate : poll.candidates) {
        if (!first) {
            oss << ",";
        }
        first = false;
        int votes = 0;
        auto it = poll.votes.find(candidate);
        if (it != poll.votes.end()) {
            votes = it->second;
        }
        oss << "{\"candidate\":" << Quote(candidate) << ",\"votes\":" << votes << "}";
    }
    oss << "]";
    return oss.str();
}

std::string BuildStringArrayJson(const std::vector<std::string> &values) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            oss << ",";
        }
        oss << Quote(values[i]);
    }
    oss << "]";
    return oss.str();
}

void ReplyJson(httplib::Response &res, int status, const std::string &json) {
    res.status = status;
    res.set_content(json, "application/json; charset=utf-8");
}

void ReplyError(httplib::Response &res, int status, const std::string &code, const std::string &message) {
    std::ostringstream oss;
    oss << "{\"error\":{\"code\":" << Quote(code) << ",\"message\":" << Quote(message) << "}}";
    ReplyJson(res, status, oss.str());
}

std::optional<std::string> ExtractJsonStringField(const std::string &body, const std::string &field) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
    std::smatch match;
    if (!std::regex_search(body, match, pattern) || match.size() < 2) {
        return std::nullopt;
    }
    return JsonUnescape(match[1].str());
}

std::optional<int> ExtractJsonIntField(const std::string &body, const std::string &field) {
    const std::regex pattern("\"" + field + R"("\s*:\s*(-?\d+))");
    std::smatch match;
    if (!std::regex_search(body, match, pattern) || match.size() < 2) {
        return std::nullopt;
    }

    try {
        return std::stoi(match[1].str());
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::vector<std::string>> ExtractJsonStringArrayField(const std::string &body, const std::string &field) {
    const std::regex array_pattern("\"" + field + R"("\s*:\s*\[([\s\S]*?)\])");
    std::smatch match;
    if (!std::regex_search(body, match, array_pattern) || match.size() < 2) {
        return std::nullopt;
    }

    std::vector<std::string> values;
    const std::string inner = match[1].str();
    const std::regex item_pattern("\"((?:\\\\.|[^\"\\\\])*)\"");
    for (auto it = std::sregex_iterator(inner.begin(), inner.end(), item_pattern); it != std::sregex_iterator(); ++it) {
        values.push_back(JsonUnescape((*it)[1].str()));
    }
    return values;
}

std::optional<std::string> GetStringField(const httplib::Request &req, const std::string &field) {
    if (req.has_param(field)) {
        return req.get_param_value(field);
    }
    return ExtractJsonStringField(req.body, field);
}

std::optional<int> GetIntField(const httplib::Request &req, const std::string &field) {
    if (req.has_param(field)) {
        try {
            return std::stoi(req.get_param_value(field));
        } catch (...) {
            return std::nullopt;
        }
    }
    return ExtractJsonIntField(req.body, field);
}

std::vector<std::string> SplitCandidates(const std::string &input) {
    std::vector<std::string> out;
    std::string token;
    char separator = '|';
    if (input.find('|') == std::string::npos && input.find(',') != std::string::npos) {
        separator = ',';
    }

    std::stringstream ss(input);
    while (std::getline(ss, token, separator)) {
        token = Trim(token);
        if (!token.empty()) {
            out.push_back(token);
        }
    }

    std::unordered_set<std::string> seen;
    std::vector<std::string> unique_out;
    unique_out.reserve(out.size());
    for (const auto &candidate : out) {
        if (seen.insert(candidate).second) {
            unique_out.push_back(candidate);
        }
    }
    return unique_out;
}

std::vector<std::string> GetCandidatesField(const httplib::Request &req) {
    if (req.has_param("candidates")) {
        return SplitCandidates(req.get_param_value("candidates"));
    }

    if (auto from_json = ExtractJsonStringArrayField(req.body, "candidates")) {
        std::vector<std::string> cleaned;
        cleaned.reserve(from_json->size());
        for (auto value : *from_json) {
            value = Trim(value);
            if (!value.empty()) {
                cleaned.push_back(value);
            }
        }

        std::unordered_set<std::string> seen;
        std::vector<std::string> unique_out;
        unique_out.reserve(cleaned.size());
        for (const auto &candidate : cleaned) {
            if (seen.insert(candidate).second) {
                unique_out.push_back(candidate);
            }
        }
        return unique_out;
    }

    return {};
}

std::vector<std::string> IssueTokens(Poll &poll, int count) {
    std::vector<std::string> tokens;
    tokens.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        std::string token;
        do {
            token = GenerateToken();
        } while (poll.issued_tokens.count(token) != 0);

        poll.issued_tokens.insert(token);
        tokens.push_back(token);
    }

    return tokens;
}

bool ParseBallot(const std::string &ballot, std::string &candidate_out, std::string &ballot_id_out) {
    const size_t pos = ballot.find('|');
    if (pos == std::string::npos || pos == 0 || pos == ballot.size() - 1) {
        return false;
    }

    candidate_out = Trim(ballot.substr(0, pos));
    ballot_id_out = Trim(ballot.substr(pos + 1));
    return !candidate_out.empty() && !ballot_id_out.empty();
}

bool CandidateExists(const Poll &poll, const std::string &candidate) {
    return std::find(poll.candidates.begin(), poll.candidates.end(), candidate) != poll.candidates.end();
}

} // namespace

int main() {
    httplib::Server server;

    server.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        ReplyJson(res, 200, R"({"status":"ok"})");
    });

    server.Post("/polls/create", [](const httplib::Request &req, httplib::Response &res) {
        auto candidates = GetCandidatesField(req);
        if (candidates.size() < 2) {
            ReplyError(res, 400, "bad_request", "Provide at least 2 candidates (candidates=A|B or JSON array).");
            return;
        }

        std::string title = "Untitled poll";
        if (auto title_field = GetStringField(req, "title")) {
            const auto trimmed = Trim(*title_field);
            if (!trimmed.empty()) {
                title = trimmed;
            }
        }

        int token_count = 0;
        if (auto parsed_count = GetIntField(req, "token_count")) {
            token_count = *parsed_count;
        }
        if (token_count < 0 || token_count > 100000) {
            ReplyError(res, 400, "bad_request", "token_count must be between 0 and 100000.");
            return;
        }

        const std::string keys = KeyGen();
        const size_t delimiter = keys.find(':');
        if (delimiter == std::string::npos || delimiter == 0 || delimiter == keys.size() - 1) {
            ReplyError(res, 500, "internal_error", "KeyGen returned invalid key pair format.");
            return;
        }

        Poll poll;
        poll.title = title;
        poll.public_key = keys.substr(0, delimiter);
        poll.secret_key = keys.substr(delimiter + 1);
        poll.candidates = std::move(candidates);
        poll.created_at = CurrentTimestampUtc();

        for (const auto &candidate : poll.candidates) {
            poll.votes[candidate] = 0;
        }

        std::vector<std::string> initial_tokens;
        if (token_count > 0) {
            initial_tokens = IssueTokens(poll, token_count);
        }

        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            do {
                poll.id = GeneratePollId();
            } while (g_polls.count(poll.id) != 0);
            g_polls.emplace(poll.id, poll);
        }

        std::ostringstream out;
        out << "{"
            << "\"poll_id\":" << Quote(poll.id) << ","
            << "\"title\":" << Quote(poll.title) << ","
            << "\"public_key\":" << Quote(poll.public_key) << ","
            << "\"created_at\":" << Quote(poll.created_at) << ","
            << "\"is_open\":true,"
            << "\"candidates\":" << BuildStringArrayJson(poll.candidates) << ","
            << "\"issued_tokens\":" << poll.issued_tokens.size() << ","
            << "\"initial_tokens\":" << BuildStringArrayJson(initial_tokens)
            << "}";

        ReplyJson(res, 201, out.str());
    });

    server.Get("/polls", [](const httplib::Request &, httplib::Response &res) {
        std::ostringstream out;
        out << "{\"polls\":[";

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        bool first = true;
        for (const auto &[poll_id, poll] : g_polls) {
            if (!first) {
                out << ",";
            }
            first = false;
            out << "{"
                << "\"poll_id\":" << Quote(poll_id) << ","
                << "\"title\":" << Quote(poll.title) << ","
                << "\"is_open\":" << (poll.is_open ? "true" : "false") << ","
                << "\"created_at\":" << Quote(poll.created_at) << ","
                << "\"candidates_count\":" << poll.candidates.size() << ","
                << "\"total_votes\":" << CountTotalVotes(poll)
                << "}";
        }

        out << "]}";
        ReplyJson(res, 200, out.str());
    });

    server.Get(R"(/polls/(poll_[A-Za-z0-9]+))", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        auto it = g_polls.find(poll_id);
        if (it == g_polls.end()) {
            ReplyError(res, 404, "not_found", "Poll not found.");
            return;
        }

        const Poll &poll = it->second;
        std::ostringstream out;
        out << "{"
            << "\"poll_id\":" << Quote(poll.id) << ","
            << "\"title\":" << Quote(poll.title) << ","
            << "\"public_key\":" << Quote(poll.public_key) << ","
            << "\"created_at\":" << Quote(poll.created_at) << ","
            << "\"is_open\":" << (poll.is_open ? "true" : "false") << ","
            << "\"candidates\":" << BuildStringArrayJson(poll.candidates) << ","
            << "\"issued_tokens\":" << poll.issued_tokens.size() << ","
            << "\"signed_tokens\":" << poll.consumed_tokens.size() << ","
            << "\"total_votes\":" << CountTotalVotes(poll) << ","
            << "\"results\":" << BuildResultsArrayJson(poll)
            << "}";

        ReplyJson(res, 200, out.str());
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/issue-token)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();
        int count = 1;
        if (auto parsed_count = GetIntField(req, "count")) {
            count = *parsed_count;
        }

        if (count < 1 || count > 100000) {
            ReplyError(res, 400, "bad_request", "count must be between 1 and 100000.");
            return;
        }

        std::vector<std::string> tokens;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            auto it = g_polls.find(poll_id);
            if (it == g_polls.end()) {
                ReplyError(res, 404, "not_found", "Poll not found.");
                return;
            }
            if (!it->second.is_open) {
                ReplyError(res, 409, "poll_closed", "Poll is closed.");
                return;
            }

            tokens = IssueTokens(it->second, count);
        }

        std::ostringstream out;
        out << "{"
            << "\"poll_id\":" << Quote(poll_id) << ","
            << "\"count\":" << tokens.size() << ","
            << "\"tokens\":" << BuildStringArrayJson(tokens)
            << "}";

        ReplyJson(res, 201, out.str());
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/sign)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        auto token = GetStringField(req, "token");
        if (!token || Trim(*token).empty()) {
            ReplyError(res, 400, "bad_request", "Field 'token' is required.");
            return;
        }

        auto blinded_message = GetStringField(req, "blinded_message");
        if (!blinded_message || blinded_message->empty()) {
            blinded_message = GetStringField(req, "blinded");
        }
        if (!blinded_message || blinded_message->empty()) {
            ReplyError(res, 400, "bad_request", "Field 'blinded_message' is required.");
            return;
        }

        std::string secret_key;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            auto it = g_polls.find(poll_id);
            if (it == g_polls.end()) {
                ReplyError(res, 404, "not_found", "Poll not found.");
                return;
            }

            Poll &poll = it->second;
            if (!poll.is_open) {
                ReplyError(res, 409, "poll_closed", "Poll is closed.");
                return;
            }

            if (poll.issued_tokens.count(*token) == 0) {
                ReplyError(res, 403, "forbidden", "Token is unknown for this poll.");
                return;
            }

            if (poll.consumed_tokens.count(*token) != 0) {
                ReplyError(res, 409, "token_used", "Token has already been used for signing.");
                return;
            }

            poll.consumed_tokens.insert(*token);
            secret_key = poll.secret_key;
        }

        const std::string blind_signature = BlindSign(secret_key, *blinded_message);
        if (blind_signature.empty()) {
            ReplyError(res, 500, "internal_error", "BlindSign returned empty signature.");
            return;
        }

        std::ostringstream out;
        out << "{"
            << "\"poll_id\":" << Quote(poll_id) << ","
            << "\"blind_signature\":" << Quote(blind_signature)
            << "}";
        ReplyJson(res, 200, out.str());
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/submit)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        auto ballot = GetStringField(req, "ballot");
        if (!ballot || ballot->empty()) {
            ballot = GetStringField(req, "message");
        }
        if (!ballot || ballot->empty()) {
            ReplyError(res, 400, "bad_request", "Field 'ballot' is required.");
            return;
        }

        auto signature = GetStringField(req, "signature");
        if (!signature || signature->empty()) {
            ReplyError(res, 400, "bad_request", "Field 'signature' is required.");
            return;
        }

        std::string candidate;
        std::string ballot_id;
        if (!ParseBallot(*ballot, candidate, ballot_id)) {
            ReplyError(res, 400, "bad_request", "Ballot must be in format 'Candidate|ballot_id'.");
            return;
        }

        int total_votes = 0;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            auto it = g_polls.find(poll_id);
            if (it == g_polls.end()) {
                ReplyError(res, 404, "not_found", "Poll not found.");
                return;
            }

            Poll &poll = it->second;
            if (!poll.is_open) {
                ReplyError(res, 409, "poll_closed", "Poll is closed.");
                return;
            }

            if (!CandidateExists(poll, candidate)) {
                ReplyError(res, 400, "bad_request", "Unknown candidate in ballot.");
                return;
            }

            if (poll.used_ballot_ids.count(ballot_id) != 0) {
                ReplyError(res, 409, "duplicate_ballot", "ballot_id has already been submitted.");
                return;
            }

            if (!Verify(poll.public_key, *ballot, *signature)) {
                ReplyError(res, 403, "invalid_signature", "Signature verification failed.");
                return;
            }

            poll.used_ballot_ids.insert(ballot_id);
            poll.votes[candidate] += 1;
            total_votes = CountTotalVotes(poll);
        }

        std::ostringstream out;
        out << "{"
            << "\"poll_id\":" << Quote(poll_id) << ","
            << "\"status\":\"accepted\","
            << "\"candidate\":" << Quote(candidate) << ","
            << "\"total_votes\":" << total_votes
            << "}";

        ReplyJson(res, 200, out.str());
    });

    server.Get(R"(/polls/(poll_[A-Za-z0-9]+)/results)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        auto it = g_polls.find(poll_id);
        if (it == g_polls.end()) {
            ReplyError(res, 404, "not_found", "Poll not found.");
            return;
        }

        const Poll &poll = it->second;
        std::ostringstream out;
        out << "{"
            << "\"poll_id\":" << Quote(poll.id) << ","
            << "\"title\":" << Quote(poll.title) << ","
            << "\"is_open\":" << (poll.is_open ? "true" : "false") << ","
            << "\"total_votes\":" << CountTotalVotes(poll) << ","
            << "\"results\":" << BuildResultsArrayJson(poll)
            << "}";

        ReplyJson(res, 200, out.str());
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/close)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        auto it = g_polls.find(poll_id);
        if (it == g_polls.end()) {
            ReplyError(res, 404, "not_found", "Poll not found.");
            return;
        }

        it->second.is_open = false;
        ReplyJson(res, 200, "{\"poll_id\":" + Quote(poll_id) + ",\"status\":\"closed\"}");
    });

    constexpr int kPort = 8080;
    std::cout << "Blind signature voting server is running on http://0.0.0.0:" << kPort << '\n';
    std::cout << "Health check: GET /health\n";
    std::cout << "Create poll: POST /polls/create\n";

    server.listen("0.0.0.0", kPort);
    return 0;
}


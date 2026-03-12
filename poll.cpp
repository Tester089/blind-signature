#include "poll.h"
#include "nlohmann_json.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>

std::unordered_map<std::string, Poll> g_polls;
std::mutex g_polls_mutex;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
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

static std::string RandomHex(size_t bytes) {
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

nlohmann::json BuildResultsArray(const Poll &poll) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &candidate : poll.candidates) {
        int votes = 0;
        auto it = poll.votes.find(candidate);
        if (it != poll.votes.end()) {
            votes = it->second;
        }
        nlohmann::json item;
        item["candidate"] = candidate;
        item["votes"] = votes;
        arr.push_back(item);
    }
    return arr;
}

nlohmann::json BuildStringArray(const std::vector<std::string> &values) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &value : values) {
        arr.push_back(value);
    }
    return arr;
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



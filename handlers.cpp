#include "handlers.h"

#include "core/core.h"

#include <iostream>

namespace {

bool ParseJsonBody(const httplib::Request &req, httplib::Response &res, JsonContext &out) {
    out = ParseJsonObject(req);
    return EnsureJsonOk(out, res);
}

bool EnsureOpenPoll(const Poll &poll, httplib::Response &res) {
    if (!poll.is_open) {
        ReplyError(res, 409, "poll_closed", "Poll is closed.");
        return false;
    }
    return true;
}

}

void RegisterHandlers(httplib::Server &server) {
    server.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        json out;
        out["status"] = "ok";
        ReplyJson(res, 200, out);
    });

    server.Post("/polls/create", [](const httplib::Request &req, httplib::Response &res) {
        JsonContext body;
        if (!ParseJsonBody(req, res, body)) {
            return;
        }

        auto candidates = GetCandidatesField(req, body.obj);
        if (candidates.size() < 2) {
            ReplyError(res, 400, "bad_request", "Need at least 2 candidates.");
            return;
        }

        std::string title = "Untitled poll";
        if (auto title_field = GetStringField(req, body.obj, "title")) {
            const auto trimmed = Trim(*title_field);
            if (!trimmed.empty()) {
                title = trimmed;
            }
        }

        int token_count = 0;
        if (auto parsed_count = GetIntField(req, body.obj, "token_count")) {
            token_count = *parsed_count;
        }
        if (token_count < 0 || token_count > kMaxTokenCount) {
            ReplyError(res, 400, "bad_request", "token_count must be 0..100000.");
            return;
        }

        const std::string keys = KeyGen();
        const size_t delimiter = keys.find(':');
        if (delimiter == std::string::npos || delimiter == 0 || delimiter == keys.size() - 1) {
            ReplyError(res, 500, "internal_error", "Key pair is invalid.");
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

        json out;
        out["poll_id"] = poll.id;
        out["title"] = poll.title;
        out["public_key"] = poll.public_key;
        out["created_at"] = poll.created_at;
        out["is_open"] = true;
        out["candidates"] = BuildStringArray(poll.candidates);
        out["issued_tokens"] = static_cast<int>(poll.issued_tokens.size());
        out["initial_tokens"] = BuildStringArray(initial_tokens);

        ReplyJson(res, 201, out);
    });

    server.Get("/polls", [](const httplib::Request &, httplib::Response &res) {
        json list = json::array();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        for (const auto &[poll_id, poll] : g_polls) {
            json item;
            item["poll_id"] = poll_id;
            item["title"] = poll.title;
            item["is_open"] = poll.is_open;
            item["created_at"] = poll.created_at;
            item["candidates_count"] = static_cast<int>(poll.candidates.size());
            item["total_votes"] = CountTotalVotes(poll);
            list.push_back(item);
        }

        json out;
        out["polls"] = list;
        ReplyJson(res, 200, out);
    });

    server.Get(R"(/polls/(poll_[A-Za-z0-9]+))", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        Poll *poll = FindPollLocked(poll_id, res);
        if (!poll) {
            return;
        }

        json out;
        out["poll_id"] = poll->id;
        out["title"] = poll->title;
        out["public_key"] = poll->public_key;
        out["created_at"] = poll->created_at;
        out["is_open"] = poll->is_open;
        out["candidates"] = BuildStringArray(poll->candidates);
        out["issued_tokens"] = static_cast<int>(poll->issued_tokens.size());
        out["signed_tokens"] = static_cast<int>(poll->consumed_tokens.size());
        out["total_votes"] = CountTotalVotes(*poll);
        out["results"] = BuildResultsArray(*poll);

        ReplyJson(res, 200, out);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/issue-token)", [](const httplib::Request &req, httplib::Response &res) {
        JsonContext body;
        if (!ParseJsonBody(req, res, body)) {
            return;
        }

        const std::string poll_id = req.matches[1].str();
        int count = 1;
        if (auto parsed_count = GetIntField(req, body.obj, "count")) {
            count = *parsed_count;
        }

        if (count < 1 || count > kMaxTokenCount) {
            ReplyError(res, 400, "bad_request", "count must be 1..100000.");
            return;
        }

        std::vector<std::string> tokens;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            Poll *poll = FindPollLocked(poll_id, res);
            if (!poll) {
                return;
            }
            if (!EnsureOpenPoll(*poll, res)) {
                return;
            }

            tokens = IssueTokens(*poll, count);
        }

        json out;
        out["poll_id"] = poll_id;
        out["count"] = static_cast<int>(tokens.size());
        out["tokens"] = BuildStringArray(tokens);

        ReplyJson(res, 201, out);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/sign)", [](const httplib::Request &req, httplib::Response &res) {
        JsonContext body;
        if (!ParseJsonBody(req, res, body)) {
            return;
        }

        const std::string poll_id = req.matches[1].str();

        auto token = GetStringField(req, body.obj, "token");
        if (!token || Trim(*token).empty()) {
            ReplyError(res, 400, "bad_request", "token is required.");
            return;
        }

        auto blinded_message = GetStringField(req, body.obj, "blinded_message");
        if (!blinded_message || blinded_message->empty()) {
            ReplyError(res, 400, "bad_request", "blinded_message is required.");
            return;
        }

        std::string secret_key;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            Poll *poll = FindPollLocked(poll_id, res);
            if (!poll) {
                return;
            }

            if (!EnsureOpenPoll(*poll, res)) {
                return;
            }

            if (poll->issued_tokens.count(*token) == 0) {
                ReplyError(res, 403, "forbidden", "Token not for this poll.");
                return;
            }

            if (poll->consumed_tokens.count(*token) != 0) {
                ReplyError(res, 409, "token_used", "Token already used.");
                return;
            }

            poll->consumed_tokens.insert(*token);
            secret_key = poll->secret_key;
        }

        const std::string blind_signature = BlindSign(secret_key, *blinded_message);
        if (blind_signature.empty()) {
            ReplyError(res, 500, "internal_error", "Signing failed.");
            return;
        }

        json out;
        out["poll_id"] = poll_id;
        out["blind_signature"] = blind_signature;
        ReplyJson(res, 200, out);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/submit)", [](const httplib::Request &req, httplib::Response &res) {
        JsonContext body;
        if (!ParseJsonBody(req, res, body)) {
            return;
        }

        const std::string poll_id = req.matches[1].str();

        auto ballot = GetStringField(req, body.obj, "ballot");
        if (!ballot || ballot->empty()) {
            ReplyError(res, 400, "bad_request", "ballot is required.");
            return;
        }

        auto signature = GetStringField(req, body.obj, "signature");
        if (!signature || signature->empty()) {
            ReplyError(res, 400, "bad_request", "signature is required.");
            return;
        }

        std::string candidate;
        std::string ballot_id;
        if (!ParseBallot(*ballot, candidate, ballot_id)) {
            ReplyError(res, 400, "bad_request", "Ballot format: Candidate|ballot_id.");
            return;
        }

        int total_votes = 0;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            Poll *poll = FindPollLocked(poll_id, res);
            if (!poll) {
                return;
            }

            if (!EnsureOpenPoll(*poll, res)) {
                return;
            }

            if (!CandidateExists(*poll, candidate)) {
                ReplyError(res, 400, "bad_request", "Unknown candidate.");
                return;
            }

            if (poll->used_ballot_ids.count(ballot_id) != 0) {
                ReplyError(res, 409, "duplicate_ballot", "Duplicate ballot_id.");
                return;
            }

            if (!Verify(poll->public_key, *ballot, *signature)) {
                ReplyError(res, 403, "invalid_signature", "Bad signature.");
                return;
            }

            poll->used_ballot_ids.insert(ballot_id);
            poll->votes[candidate] += 1;
            total_votes = CountTotalVotes(*poll);
        }

        json out;
        out["poll_id"] = poll_id;
        out["status"] = "accepted";
        out["candidate"] = candidate;
        out["total_votes"] = total_votes;

        ReplyJson(res, 200, out);
    });

    server.Get(R"(/polls/(poll_[A-Za-z0-9]+)/results)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        Poll *poll = FindPollLocked(poll_id, res);
        if (!poll) {
            return;
        }

        json out;
        out["poll_id"] = poll->id;
        out["title"] = poll->title;
        out["is_open"] = poll->is_open;
        out["total_votes"] = CountTotalVotes(*poll);
        out["results"] = BuildResultsArray(*poll);

        ReplyJson(res, 200, out);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/close)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();

        std::lock_guard<std::mutex> lock(g_polls_mutex);
        Poll *poll = FindPollLocked(poll_id, res);
        if (!poll) {
            return;
        }

        poll->is_open = false;

        json out;
        out["poll_id"] = poll_id;
        out["status"] = "closed";
        ReplyJson(res, 200, out);
    });
}

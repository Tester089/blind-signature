#include "handlers.h"

#include "core.h"

#include <iostream>

static json BuildAllowedArray(const Poll &poll) {
    json arr = json::array();
    for (const auto &u : poll.allowed_usernames) arr.push_back(u);
    return arr;
}

void RegisterHandlers(httplib::Server &server) {
    server.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        json root; root["status"] = "ok"; ReplyJson(res, 200, root);
    });

    server.Post("/polls/create", [](const httplib::Request &req, httplib::Response &res) {
        auto ctx = ParseJsonObject(req);
        if (!EnsureJsonOk(ctx, res)) return;
        auto candidates = GetCandidatesField(req, ctx.obj);
        if (candidates.size() < 2) {
            ReplyError(res, 400, "bad_request", "Provide at least 2 candidates.");
            return;
        }
        std::string title = "Untitled poll";
        if (auto title_field = GetStringField(req, ctx.obj, "title")) {
            const auto trimmed = Trim(*title_field); if (!trimmed.empty()) title = trimmed;
        }
        std::string owner_username;
        if (auto owner = GetStringField(req, ctx.obj, "owner_username")) owner_username = Trim(*owner);
        auto allowed = GetStringArrayField(req, ctx.obj, "allowed_usernames");
        int token_count = 0;
        if (auto parsed_count = GetIntField(req, ctx.obj, "token_count")) token_count = *parsed_count;
        if (token_count < 0 || token_count > kMaxTokenCount) {
            ReplyError(res, 400, "bad_request", "token_count must be between 0 and 100000.");
            return;
        }
        KeyPair keys = KeyGen();
        if (!keys.pk || !keys.sk) { ReplyError(res, 500, "internal_error", "KeyGen returned invalid key pair."); return; }
        Poll poll;
        poll.title = title;
        poll.public_key = keys.pk->toString();
        poll.secret_key = keys.sk->toString();
        poll.candidates = std::move(candidates);
        poll.created_at = CurrentTimestampUtc();
        poll.owner_username = owner_username;
        for (const auto &u : allowed) poll.allowed_usernames.insert(u);
        for (const auto &candidate : poll.candidates) poll.votes[candidate] = 0;
        std::vector<std::string> initial_tokens;
        if (token_count > 0) initial_tokens = IssueTokens(poll, token_count);
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            do { poll.id = GeneratePollId(); } while (g_polls.count(poll.id) != 0);
            g_polls.emplace(poll.id, poll);
        }
        json root;
        root["poll_id"] = poll.id; root["title"] = poll.title; root["public_key"] = poll.public_key; root["created_at"] = poll.created_at;
        root["is_open"] = true; root["candidates"] = BuildStringArray(poll.candidates); root["issued_tokens"] = (int)poll.issued_tokens.size();
        root["initial_tokens"] = BuildStringArray(initial_tokens); root["owner_username"] = poll.owner_username; root["allowed_usernames"] = BuildAllowedArray(poll);
        ReplyJson(res, 201, root);
    });

    server.Get("/polls", [](const httplib::Request &, httplib::Response &res) {
        json polls = json::array();
        std::lock_guard<std::mutex> lock(g_polls_mutex);
        for (const auto &[poll_id, poll] : g_polls) {
            json item;
            item["poll_id"] = poll_id; item["title"] = poll.title; item["is_open"] = poll.is_open; item["created_at"] = poll.created_at;
            item["candidates_count"] = (int)poll.candidates.size(); item["total_votes"] = CountTotalVotes(poll); item["owner_username"] = poll.owner_username;
            item["allowed_voters_count"] = (int)poll.allowed_usernames.size();
            polls.push_back(item);
        }
        json root; root["polls"] = polls; ReplyJson(res, 200, root);
    });

    server.Get(R"(/polls/(poll_[A-Za-z0-9]+))", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();
        std::lock_guard<std::mutex> lock(g_polls_mutex);
        Poll *poll_ptr = FindPollLocked(poll_id, res); if (!poll_ptr) return;
        const Poll &poll = *poll_ptr;
        json root;
        root["poll_id"] = poll.id; root["title"] = poll.title; root["public_key"] = poll.public_key; root["created_at"] = poll.created_at;
        root["is_open"] = poll.is_open; root["candidates"] = BuildStringArray(poll.candidates); root["issued_tokens"] = (int)poll.issued_tokens.size();
        root["signed_tokens"] = (int)poll.consumed_tokens.size(); root["total_votes"] = CountTotalVotes(poll); root["results"] = BuildResultsArray(poll);
        root["owner_username"] = poll.owner_username; root["allowed_usernames"] = BuildAllowedArray(poll); root["voted_usernames_count"] = (int)poll.voted_usernames.size();
        ReplyJson(res, 200, root);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/issue-token)", [](const httplib::Request &req, httplib::Response &res) {
        auto ctx = ParseJsonObject(req); if (!EnsureJsonOk(ctx, res)) return;
        const std::string poll_id = req.matches[1].str();
        int count = 1; if (auto parsed_count = GetIntField(req, ctx.obj, "count")) count = *parsed_count;
        std::string username; if (auto u = GetStringField(req, ctx.obj, "username")) username = Trim(*u);
        if (username.empty()) { ReplyError(res, 400, "bad_request", "Field 'username' is required."); return; }
        if (count < 1 || count > kMaxTokenCount) { ReplyError(res, 400, "bad_request", "count must be between 1 and 100000."); return; }
        std::vector<std::string> tokens;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            Poll *poll = FindPollLocked(poll_id, res); if (!poll) return;
            if (!poll->is_open) { ReplyError(res, 409, "poll_closed", "Poll is closed."); return; }
            if (!poll->allowed_usernames.empty() && poll->allowed_usernames.count(username) == 0) { ReplyError(res, 403, "not_allowed", "User is not allowed to vote in this poll."); return; }
            if (poll->voted_usernames.count(username) != 0) { ReplyError(res, 409, "already_voted", "User has already voted in this poll."); return; }
            for (const auto &[tok, owner] : poll->issued_token_owner) if (owner == username && poll->submitted_tokens.count(tok) == 0) {
                ReplyError(res, 409, "token_exists", "User already has an active token for this poll."); return; }
            tokens = IssueTokens(*poll, count);
            for (const auto &tok : tokens) poll->issued_token_owner[tok] = username;
        }
        json root; root["poll_id"] = poll_id; root["count"] = (int)tokens.size(); root["tokens"] = BuildStringArray(tokens); root["username"] = username; ReplyJson(res, 201, root);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/sign)", [](const httplib::Request &req, httplib::Response &res) {
        auto ctx = ParseJsonObject(req); if (!EnsureJsonOk(ctx, res)) return;
        const std::string poll_id = req.matches[1].str();
        auto token = GetStringField(req, ctx.obj, "token"); if (!token || Trim(*token).empty()) { ReplyError(res, 400, "bad_request", "Field 'token' is required."); return; }
        auto blinded_message = GetStringField(req, ctx.obj, "blinded_message"); if (!blinded_message || blinded_message->empty()) { ReplyError(res, 400, "bad_request", "Field 'blinded_message' is required."); return; }
        std::string secret_key;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            Poll *poll = FindPollLocked(poll_id, res); if (!poll) return;
            if (!poll->is_open) { ReplyError(res, 409, "poll_closed", "Poll is closed."); return; }
            if (poll->issued_tokens.count(*token) == 0) { ReplyError(res, 403, "forbidden", "Token is unknown for this poll."); return; }
            if (poll->consumed_tokens.count(*token) != 0) { ReplyError(res, 409, "token_used", "Token has already been used for signing."); return; }
            poll->consumed_tokens.insert(*token);
            secret_key = poll->secret_key;
        }
        const std::string blind_signature = BlindSign(secret_key, *blinded_message);
        if (blind_signature.empty()) { ReplyError(res, 500, "internal_error", "BlindSign returned empty signature."); return; }
        json root; root["poll_id"] = poll_id; root["blind_signature"] = blind_signature; ReplyJson(res, 200, root);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/submit)", [](const httplib::Request &req, httplib::Response &res) {
        auto ctx = ParseJsonObject(req); if (!EnsureJsonOk(ctx, res)) return;
        const std::string poll_id = req.matches[1].str();
        auto ballot = GetStringField(req, ctx.obj, "ballot"); if (!ballot || ballot->empty()) { ReplyError(res, 400, "bad_request", "Field 'ballot' is required."); return; }
        auto signature = GetStringField(req, ctx.obj, "signature"); if (!signature || signature->empty()) { ReplyError(res, 400, "bad_request", "Field 'signature' is required."); return; }
        auto token = GetStringField(req, ctx.obj, "token"); if (!token || token->empty()) { ReplyError(res, 400, "bad_request", "Field 'token' is required."); return; }
        auto usernameField = GetStringField(req, ctx.obj, "username"); if (!usernameField || Trim(*usernameField).empty()) { ReplyError(res, 400, "bad_request", "Field 'username' is required."); return; }
        std::string username = Trim(*usernameField);
        std::string candidate, ballot_id;
        if (!ParseBallot(*ballot, candidate, ballot_id)) { ReplyError(res, 400, "bad_request", "Ballot must be in format 'Candidate|ballot_id'."); return; }
        int total_votes = 0;
        {
            std::lock_guard<std::mutex> lock(g_polls_mutex);
            Poll *poll = FindPollLocked(poll_id, res); if (!poll) return;
            if (!poll->is_open) { ReplyError(res, 409, "poll_closed", "Poll is closed."); return; }
            if (!CandidateExists(*poll, candidate)) { ReplyError(res, 400, "bad_request", "Unknown candidate in ballot."); return; }
            if (!poll->allowed_usernames.empty() && poll->allowed_usernames.count(username) == 0) { ReplyError(res, 403, "not_allowed", "User is not allowed to vote in this poll."); return; }
            if (poll->used_ballot_ids.count(ballot_id) != 0) { ReplyError(res, 409, "duplicate_ballot", "ballot_id has already been submitted."); return; }
            if (poll->voted_usernames.count(username) != 0) { ReplyError(res, 409, "already_voted", "User has already voted in this poll."); return; }
            if (poll->issued_tokens.count(*token) == 0 || poll->consumed_tokens.count(*token) == 0) { ReplyError(res, 403, "invalid_token", "Token is not valid for submit."); return; }
            auto tokIt = poll->issued_token_owner.find(*token);
            if (tokIt == poll->issued_token_owner.end() || tokIt->second != username) { ReplyError(res, 403, "invalid_token_owner", "Token does not belong to this user."); return; }
            if (poll->submitted_tokens.count(*token) != 0) { ReplyError(res, 409, "token_already_submitted", "Token was already used for submit."); return; }
            if (!Verify(poll->public_key, *ballot, *signature)) { ReplyError(res, 403, "invalid_signature", "Signature verification failed."); return; }
            poll->used_ballot_ids.insert(ballot_id); poll->votes[candidate] += 1; poll->voted_usernames.insert(username); poll->submitted_tokens.insert(*token); total_votes = CountTotalVotes(*poll);
        }
        json root; root["poll_id"] = poll_id; root["status"] = "accepted"; root["candidate"] = candidate; root["total_votes"] = total_votes; root["username"] = username; ReplyJson(res, 200, root);
    });

    server.Get(R"(/polls/(poll_[A-Za-z0-9]+)/results)", [](const httplib::Request &req, httplib::Response &res) {
        const std::string poll_id = req.matches[1].str();
        std::lock_guard<std::mutex> lock(g_polls_mutex);
        Poll *poll_ptr = FindPollLocked(poll_id, res); if (!poll_ptr) return;
        const Poll &poll = *poll_ptr; json root; root["poll_id"] = poll.id; root["title"] = poll.title; root["is_open"] = poll.is_open; root["total_votes"] = CountTotalVotes(poll); root["results"] = BuildResultsArray(poll); ReplyJson(res, 200, root);
    });

    server.Post(R"(/polls/(poll_[A-Za-z0-9]+)/close)", [](const httplib::Request &req, httplib::Response &res) {
        auto ctx = ParseJsonObject(req); if (!EnsureJsonOk(ctx, res)) return; const std::string poll_id = req.matches[1].str();
        std::string username; if (auto u = GetStringField(req, ctx.obj, "username")) username = Trim(*u);
        std::lock_guard<std::mutex> lock(g_polls_mutex); Poll *poll = FindPollLocked(poll_id, res); if (!poll) return;
        if (!poll->owner_username.empty() && poll->owner_username != username) { ReplyError(res, 403, "forbidden", "Only the poll owner can close this poll."); return; }
        poll->is_open = false; json root; root["poll_id"] = poll_id; root["status"] = "closed"; ReplyJson(res, 200, root);
    });
}

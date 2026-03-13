#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Poll {
    std::string id;
    std::string title;
    std::string public_key;
    std::string secret_key;
    std::string created_at;
    bool is_open = true;
    std::string owner_username;
    std::vector<std::string> candidates;
    std::unordered_map<std::string, int> votes;
    std::unordered_set<std::string> allowed_usernames;
    std::unordered_map<std::string, std::string> issued_token_owner; // token -> username
    std::unordered_set<std::string> issued_tokens;
    std::unordered_set<std::string> consumed_tokens;
    std::unordered_set<std::string> submitted_tokens;
    std::unordered_set<std::string> used_ballot_ids;
    std::unordered_set<std::string> voted_usernames;
};

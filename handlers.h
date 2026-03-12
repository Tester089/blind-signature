#pragma once

#ifdef __cplusplus // for some reason by default it's defining like C 💀💀💀

#include "poll.h"
#include "json_utils.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int kMaxTokenCount = 100000;
constexpr size_t kPollIdBytes = 8;
constexpr size_t kTokenBytes = 16;

extern std::unordered_map<std::string, Poll> g_polls;
extern std::mutex g_polls_mutex;

std::string Trim(std::string value);
std::string CurrentTimestampUtc();
std::string GeneratePollId();
std::string GenerateToken();

int CountTotalVotes(const Poll &poll);
json BuildResultsArray(const Poll &poll);
json BuildStringArray(const std::vector<std::string> &values);

std::vector<std::string> IssueTokens(Poll &poll, int count);
bool ParseBallot(const std::string &ballot, std::string &candidate_out, std::string &ballot_id_out);
bool CandidateExists(const Poll &poll, const std::string &candidate);

void RegisterHandlers(httplib::Server &server);

#endif

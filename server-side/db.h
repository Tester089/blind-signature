#pragma once

#include <string>
#include <vector>

bool DbEnabled();
bool DbReady(std::string &error);
bool DbUserExists(const std::string &username, bool &exists, std::string &error);
bool DbCheckUserPassword(const std::string &username, const std::string &password, bool &valid, std::string &error);
bool DbCreateUser(const std::string &username, const std::string &password, bool &created, std::string &error);
bool DbListUsers(std::vector<std::string> &users, std::string &error);
bool DbRecordVote(const std::string &poll_id, const std::string &candidate, const std::string &username, std::string &error);

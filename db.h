#pragma once

#include <string>

bool DbEnabled();
bool DbReady(std::string &error);
bool DbUserExists(const std::string &username, bool &exists, std::string &error);
bool DbCheckUserPassword(const std::string &username, const std::string &password, bool &valid, std::string &error);
bool DbRecordVote(const std::string &poll_id, const std::string &candidate, const std::string &username, std::string &error);

#pragma once

#include "httplib.h"
#include "nlohmann_json.hpp"
#include "polls.h"

#include <optional>
#include <string>
#include <vector>

using json = nlohmann::json;

struct JsonContext {
    std::optional<json> storage;
    const json *obj = nullptr;
    bool ok = true;
};

JsonContext ParseJsonObject(const httplib::Request &req);
bool EnsureJsonOk(const JsonContext &ctx, httplib::Response &res);

void ReplyJson(httplib::Response &res, int status, const json &value);
void ReplyError(httplib::Response &res, int status, const std::string &code, const std::string &message);

std::optional<std::string> GetStringField(const httplib::Request &req, const json *obj, const std::string &field);
std::optional<int> GetIntField(const httplib::Request &req, const json *obj, const std::string &field);
std::vector<std::string> GetCandidatesField(const httplib::Request &req, const json *obj);
std::vector<std::string> GetStringArrayField(const httplib::Request &req, const json *obj, const std::string &field);

Poll *FindPollLocked(const std::string &poll_id, httplib::Response &res);

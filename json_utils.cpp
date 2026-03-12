#include "json_utils.h"
#include "handlers.h"

#include <sstream>
#include <unordered_set>

JsonContext ParseJsonObject(const httplib::Request &req) {
    JsonContext ctx;
    if (req.body.empty()) {
        ctx.ok = false;
        return ctx;
    }

    try {
        auto parsed = json::parse(req.body);
        if (!parsed.is_object()) {
            ctx.ok = false;
            return ctx;
        }
        ctx.storage = std::move(parsed);
        ctx.obj = &(*ctx.storage);
        return ctx;
    } catch (...) {
        ctx.ok = false;
        return ctx;
    }
}

bool EnsureJsonOk(const JsonContext &ctx, httplib::Response &res) {
    if (!ctx.ok) {
        ReplyError(res, 400, "bad_json", "Expected a JSON object.");
        return false;
    }
    return true;
}

void ReplyJson(httplib::Response &res, int status, const json &value) {
    res.status = status;
    res.set_content(value.dump(), "application/json; charset=utf-8");
}

void ReplyError(httplib::Response &res, int status, const std::string &code, const std::string &message) {
    json root;
    root["error"]["code"] = code;
    root["error"]["message"] = message;
    ReplyJson(res, status, root);
}

Poll *FindPollLocked(const std::string &poll_id, httplib::Response &res) {
    auto it = g_polls.find(poll_id);
    if (it == g_polls.end()) {
        ReplyError(res, 404, "not_found", "No such poll.");
        return nullptr;
    }
    return &it->second;
}

std::optional<std::string> GetStringField(
    const httplib::Request &req,
    const json *obj,
    const std::string &field
) {
    (void)req;
    if (!obj || !obj->is_object()) {
        return std::nullopt;
    }
    auto it = obj->find(field);
    if (it == obj->end()) {
        return std::nullopt;
    }
    if (it->is_string()) {
        return it->get<std::string>();
    }
    return std::nullopt;
}

std::optional<int> GetIntField(
    const httplib::Request &req,
    const json *obj,
    const std::string &field
) {
    (void)req;
    if (!obj || !obj->is_object()) {
        return std::nullopt;
    }
    auto it = obj->find(field);
    if (it == obj->end()) {
        return std::nullopt;
    }
    if (it->is_number_integer()) {
        return it->get<int>();
    }
    if (it->is_number()) {
        return static_cast<int>(it->get<double>());
    }
    if (it->is_string()) {
        try {
            return std::stoi(it->get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

static std::vector<std::string> SplitCandidates(const std::string &input) {
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

std::vector<std::string> GetCandidatesField(const httplib::Request &req, const json *obj) {
    (void)req;
    if (!obj || !obj->is_object()) {
        return {};
    }

    auto it = obj->find("candidates");
    if (it == obj->end()) {
        return {};
    }

    if (it->is_string()) {
        return SplitCandidates(it->get<std::string>());
    }

    if (it->is_array()) {
        std::vector<std::string> cleaned;
        cleaned.reserve(it->size());
        for (const auto &value : *it) {
            if (value.is_string()) {
                auto trimmed = Trim(value.get<std::string>());
                if (!trimmed.empty()) {
                    cleaned.push_back(trimmed);
                }
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


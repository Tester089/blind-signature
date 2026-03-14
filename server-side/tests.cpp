#include "handlers.h"

#include <gtest/gtest.h>

TEST(PollTest, TrimRemovesSpaces) {
    EXPECT_EQ(Trim("  hello  "), std::string("hello"));
    EXPECT_EQ(Trim("\t\nvalue\r"), std::string("value"));
    EXPECT_EQ(Trim(""), std::string(""));
}

TEST(PollTest, ParseBallotValidAndInvalid) {
    std::string candidate;
    std::string ballot_id;

    EXPECT_TRUE(ParseBallot("Alice|id123", candidate, ballot_id));
    EXPECT_EQ(candidate, std::string("Alice"));
    EXPECT_EQ(ballot_id, std::string("id123"));

    EXPECT_FALSE(ParseBallot("bad", candidate, ballot_id));
    EXPECT_FALSE(ParseBallot("|id", candidate, ballot_id));
    EXPECT_FALSE(ParseBallot("Alice|", candidate, ballot_id));
}

TEST(PollTest, CandidateExistsMatchesCandidates) {
    Poll poll;
    poll.candidates = {"A", "B"};

    EXPECT_TRUE(CandidateExists(poll, "A"));
    EXPECT_FALSE(CandidateExists(poll, "C"));
}

TEST(PollTest, BuildStringArrayPreservesOrder) {
    std::vector<std::string> values = {"x", "y"};
    json arr = BuildStringArray(values);

    ASSERT_TRUE(arr.is_array());
    ASSERT_EQ(arr.size(), static_cast<size_t>(2));
    EXPECT_EQ(arr[0].dump(), "\"x\"");
    EXPECT_EQ(arr[1].dump(), "\"y\"");
}

TEST(PollTest, CountTotalVotesIgnoresUnknownCandidates) {
    Poll poll;
    poll.candidates = {"A", "B"};
    poll.votes["A"] = 2;
    poll.votes["B"] = 1;
    poll.votes["C"] = 5;

    EXPECT_EQ(CountTotalVotes(poll), 3);
}

TEST(PollTest, BuildResultsArrayReturnsCandidateVotes) {
    Poll poll;
    poll.candidates = {"A", "B"};
    poll.votes["B"] = 2;

    json arr = BuildResultsArray(poll);
    ASSERT_TRUE(arr.is_array());
    ASSERT_EQ(arr.size(), static_cast<size_t>(2));
    EXPECT_EQ(arr[0]["candidate"].dump(), "\"A\"");
    EXPECT_EQ(arr[0]["votes"].dump(), "0");
    EXPECT_EQ(arr[1]["candidate"].dump(), "\"B\"");
    EXPECT_EQ(arr[1]["votes"].dump(), "2");
}

TEST(JsonUtilsTest, ParseJsonObjectValidatesObject) {
    httplib::Request req;
    req.body = R"({"a":1})";

    JsonContext ctx = ParseJsonObject(req);
    EXPECT_TRUE(ctx.ok);
    ASSERT_NE(ctx.obj, nullptr);
    EXPECT_TRUE(ctx.obj->is_object());

    httplib::Request bad_req;
    bad_req.body = R"([1,2,3])";
    JsonContext bad_ctx = ParseJsonObject(bad_req);
    EXPECT_FALSE(bad_ctx.ok);

    httplib::Response res;
    EXPECT_FALSE(EnsureJsonOk(bad_ctx, res));
    EXPECT_EQ(res.status, 400);
    EXPECT_NE(res.body.find("bad_json"), std::string::npos);
}

TEST(JsonUtilsTest, GetStringAndIntFieldParsesTypes) {
    httplib::Request req;
    json obj = json::parse(R"({"name":"Bob","age":10,"count":"12"})");

    auto name = GetStringField(req, &obj, "name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, std::string("Bob"));
    EXPECT_FALSE(GetStringField(req, &obj, "age").has_value());

    auto age = GetIntField(req, &obj, "age");
    ASSERT_TRUE(age.has_value());
    EXPECT_EQ(*age, 10);

    auto count = GetIntField(req, &obj, "count");
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 12);
}

TEST(JsonUtilsTest, GetCandidatesFieldDeduplicatesAndTrims) {
    httplib::Request req;

    json obj_string = json::parse(R"({"candidates":" Alice | Bob |Alice "})");

    auto candidates = GetCandidatesField(req, &obj_string);
    ASSERT_EQ(candidates.size(), static_cast<size_t>(2));
    EXPECT_EQ(candidates[0], std::string("Alice"));
    EXPECT_EQ(candidates[1], std::string("Bob"));

    json obj_array = json::parse(R"({"candidates":[" Ann ","Bob","Bob",5]})");

    auto candidates2 = GetCandidatesField(req, &obj_array);
    ASSERT_EQ(candidates2.size(), static_cast<size_t>(2));
    EXPECT_EQ(candidates2[0], std::string("Ann"));
    EXPECT_EQ(candidates2[1], std::string("Bob"));
}

TEST(JsonUtilsTest, FindPollLockedFindsOrReturns404) {
    std::lock_guard<std::mutex> lock(g_polls_mutex);
    g_polls.clear();

    Poll poll;
    poll.id = "poll_test";
    g_polls.emplace(poll.id, poll);

    httplib::Response ok_res;
    Poll *found = FindPollLocked("poll_test", ok_res);
    EXPECT_NE(found, nullptr);

    httplib::Response miss_res;
    Poll *missing = FindPollLocked("poll_missing", miss_res);
    EXPECT_EQ(missing, nullptr);
    EXPECT_EQ(miss_res.status, 404);
    EXPECT_NE(miss_res.body.find("not_found"), std::string::npos);
}

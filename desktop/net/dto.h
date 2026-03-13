#pragma once

#include <QJsonArray>
#include <QString>
#include <QStringList>

struct PollSummary {
    QString pollId;
    QString title;
    bool isOpen = false;
    QString createdAt;
    int candidatesCount = 0;
    int totalVotes = 0;
    QString ownerUsername;
    int allowedVotersCount = 0;
};

struct PollDetails {
    QString pollId;
    QString title;
    QString publicKey;
    QString createdAt;
    bool isOpen = false;
    QStringList candidates;
    int issuedTokens = 0;
    int signedTokens = 0;
    int totalVotes = 0;
    QString ownerUsername;
    QStringList allowedUsernames;
    int votedUsernamesCount = 0;
    QJsonArray results;
};

struct CreatePollResponse {
    QString pollId;
    QString title;
    QString publicKey;
    QString createdAt;
    bool isOpen = false;
    QStringList candidates;
    int issuedTokens = 0;
    QStringList initialTokens;
    QString ownerUsername;
    QStringList allowedUsernames;
};

struct IssueTokenResponse {
    QString pollId;
    int count = 0;
    QStringList tokens;
    QString username;
};

struct SignResponse {
    QString pollId;
    QString blindSignature;
};

struct SubmitVoteResponse {
    QString pollId;
    QString status;
    QString candidate;
    int totalVotes = 0;
    QString username;
};

struct ResultsResponse {
    QString pollId;
    QString title;
    bool isOpen = false;
    int totalVotes = 0;
    QJsonArray results;
};

struct ClosePollResponse {
    QString pollId;
    QString status;
};

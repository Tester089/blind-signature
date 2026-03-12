#pragma once

#include <functional>

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QJsonObject>

#include "dto.h"

class QNetworkReply;

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);

    void setBaseUrl(const QUrl &url);
    QUrl baseUrl() const;

    void health();
    void listPolls();
    void getPoll(const QString &pollId);

    void createPoll(const QString &title,
                    const QStringList &candidates,
                    int tokenCount);

    void issueToken(const QString &pollId, int count = 1);

    void signBlinded(const QString &pollId,
                     const QString &token,
                     const QString &blindedMessage);

    void submitVote(const QString &pollId,
                    const QString &ballot,
                    const QString &signature);

    void getResults(const QString &pollId);
    void closePoll(const QString &pollId);

signals:
    void healthOk();
    void pollsListed(const QList<PollSummary> &polls);
    void pollFetched(const PollDetails &poll);
    void pollCreated(const CreatePollResponse &response);
    void tokenIssued(const IssueTokenResponse &response);
    void blindedSigned(const SignResponse &response);
    void voteSubmitted(const SubmitVoteResponse &response);
    void resultsReceived(const ResultsResponse &response);
    void pollClosed(const ClosePollResponse &response);
    void requestFailed(const QString &where,
                       int httpStatus,
                       const QString &errorCode,
                       const QString &message);

private:
    void handleJsonReply(QNetworkReply *reply,
                         const QString &where,
                         const std::function<void(const QJsonObject &)> &onSuccess);

    static QStringList jsonArrayToStringList(const QJsonArray &array);
    static PollSummary parsePollSummary(const QJsonObject &obj);
    static PollDetails parsePollDetails(const QJsonObject &obj);
    static CreatePollResponse parseCreatePollResponse(const QJsonObject &obj);
    static IssueTokenResponse parseIssueTokenResponse(const QJsonObject &obj);
    static SignResponse parseSignResponse(const QJsonObject &obj);
    static SubmitVoteResponse parseSubmitVoteResponse(const QJsonObject &obj);
    static ResultsResponse parseResultsResponse(const QJsonObject &obj);
    static ClosePollResponse parseClosePollResponse(const QJsonObject &obj);

    static QUrl normalizeBaseUrl(const QUrl &url);
    QUrl makeUrl(const QString &path) const;

private:
    QNetworkAccessManager manager_;
    QUrl baseUrl_;
};

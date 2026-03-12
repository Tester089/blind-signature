#include "api_client.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {
QByteArray makeJsonBody(const QJsonObject &obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent),
      baseUrl_(normalizeBaseUrl(QUrl(QStringLiteral("http://127.0.0.1:8080/"))))
{
}

QUrl ApiClient::normalizeBaseUrl(const QUrl &url)
{
    QUrl normalized = url;

    if (normalized.scheme().isEmpty()) {
        normalized = QUrl(QStringLiteral("http://") + url.toString());
    }

    if (normalized.host().isEmpty() && !normalized.path().isEmpty()) {
        normalized = QUrl(QStringLiteral("http://") + normalized.path());
    }

    if (normalized.scheme().isEmpty()) {
        normalized.setScheme(QStringLiteral("http"));
    }

    if (normalized.port() == -1) {
        normalized.setPort(8080);
    }

    normalized.setPath(QStringLiteral("/"));
    return normalized;
}

void ApiClient::setBaseUrl(const QUrl &url)
{
    baseUrl_ = normalizeBaseUrl(url);
}

QUrl ApiClient::baseUrl() const
{
    return baseUrl_;
}

QUrl ApiClient::makeUrl(const QString &path) const
{
    QUrl url = baseUrl_;
    QString cleanPath = path;
    if (cleanPath.startsWith('/')) {
        cleanPath.remove(0, 1);
    }
    url.setPath('/' + cleanPath);
    return url;
}

void ApiClient::health()
{
    QNetworkRequest req(makeUrl("/health"));
    auto *reply = manager_.get(req);

    handleJsonReply(reply, "health", [this](const QJsonObject &obj) {
        if (obj.value("status").toString() == "ok") {
            emit healthOk();
        } else {
            emit requestFailed("health", 200, "bad_response", "Unexpected health response.");
        }
    });
}

void ApiClient::listPolls()
{
    QNetworkRequest req(makeUrl("/polls"));
    auto *reply = manager_.get(req);

    handleJsonReply(reply, "listPolls", [this](const QJsonObject &obj) {
        QList<PollSummary> polls;
        const QJsonArray arr = obj.value("polls").toArray();
        polls.reserve(arr.size());
        for (const QJsonValue &v : arr) {
            if (v.isObject()) {
                polls.push_back(parsePollSummary(v.toObject()));
            }
        }
        emit pollsListed(polls);
    });
}

void ApiClient::getPoll(const QString &pollId)
{
    QNetworkRequest req(makeUrl(QString("/polls/%1").arg(pollId)));
    auto *reply = manager_.get(req);

    handleJsonReply(reply, "getPoll", [this](const QJsonObject &obj) {
        emit pollFetched(parsePollDetails(obj));
    });
}

void ApiClient::createPoll(const QString &title, const QStringList &candidates, int tokenCount)
{
    QNetworkRequest req(makeUrl("/polls/create"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonArray candidatesJson;
    for (const QString &candidate : candidates) {
        candidatesJson.push_back(candidate);
    }

    QJsonObject body{{"title", title}, {"candidates", candidatesJson}, {"token_count", tokenCount}};
    auto *reply = manager_.post(req, makeJsonBody(body));

    handleJsonReply(reply, "createPoll", [this](const QJsonObject &obj) {
        emit pollCreated(parseCreatePollResponse(obj));
    });
}

void ApiClient::issueToken(const QString &pollId, int count)
{
    QNetworkRequest req(makeUrl(QString("/polls/%1/issue-token").arg(pollId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body{{"count", count}};
    auto *reply = manager_.post(req, makeJsonBody(body));

    handleJsonReply(reply, "issueToken", [this](const QJsonObject &obj) {
        emit tokenIssued(parseIssueTokenResponse(obj));
    });
}

void ApiClient::signBlinded(const QString &pollId, const QString &token, const QString &blindedMessage)
{
    QNetworkRequest req(makeUrl(QString("/polls/%1/sign").arg(pollId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body{{"token", token}, {"blinded_message", blindedMessage}};
    auto *reply = manager_.post(req, makeJsonBody(body));

    handleJsonReply(reply, "signBlinded", [this](const QJsonObject &obj) {
        emit blindedSigned(parseSignResponse(obj));
    });
}

void ApiClient::submitVote(const QString &pollId, const QString &ballot, const QString &signature)
{
    QNetworkRequest req(makeUrl(QString("/polls/%1/submit").arg(pollId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body{{"ballot", ballot}, {"signature", signature}};
    auto *reply = manager_.post(req, makeJsonBody(body));

    handleJsonReply(reply, "submitVote", [this](const QJsonObject &obj) {
        emit voteSubmitted(parseSubmitVoteResponse(obj));
    });
}

void ApiClient::getResults(const QString &pollId)
{
    QNetworkRequest req(makeUrl(QString("/polls/%1/results").arg(pollId)));
    auto *reply = manager_.get(req);

    handleJsonReply(reply, "getResults", [this](const QJsonObject &obj) {
        emit resultsReceived(parseResultsResponse(obj));
    });
}

void ApiClient::closePoll(const QString &pollId)
{
    QNetworkRequest req(makeUrl(QString("/polls/%1/close").arg(pollId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto *reply = manager_.post(req, QByteArray{});

    handleJsonReply(reply, "closePoll", [this](const QJsonObject &obj) {
        emit pollClosed(parseClosePollResponse(obj));
    });
}

void ApiClient::handleJsonReply(QNetworkReply *reply,
                                const QString &where,
                                const std::function<void(const QJsonObject &)> &onSuccess)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, where, onSuccess]() {
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();
        const auto parseResult = QJsonDocument::fromJson(raw);

        if (reply->error() != QNetworkReply::NoError) {
            QString errorCode = QStringLiteral("network_error");
            QString message = reply->errorString();
            if (parseResult.isObject()) {
                const QJsonObject obj = parseResult.object();
                if (obj.contains("error")) {
                    errorCode = obj.value("error").toString(errorCode);
                }
                if (obj.contains("message")) {
                    message = obj.value("message").toString(message);
                }
            }
            emit requestFailed(where, httpStatus, errorCode, message);
            reply->deleteLater();
            return;
        }

        if (!parseResult.isObject()) {
            emit requestFailed(where, httpStatus, "bad_json", "Response is not a JSON object.");
            reply->deleteLater();
            return;
        }

        const QJsonObject obj = parseResult.object();
        if (httpStatus >= 400) {
            emit requestFailed(where,
                               httpStatus,
                               obj.value("error").toString("http_error"),
                               obj.value("message").toString("HTTP request failed."));
            reply->deleteLater();
            return;
        }

        onSuccess(obj);
        reply->deleteLater();
    });
}

QStringList ApiClient::jsonArrayToStringList(const QJsonArray &array)
{
    QStringList out;
    out.reserve(array.size());
    for (const QJsonValue &v : array) {
        out.push_back(v.toString());
    }
    return out;
}

PollSummary ApiClient::parsePollSummary(const QJsonObject &obj)
{
    PollSummary p;
    p.pollId = obj.value("poll_id").toString();
    p.title = obj.value("title").toString();
    p.isOpen = obj.value("is_open").toBool();
    p.createdAt = obj.value("created_at").toString();
    p.candidatesCount = obj.value("candidates_count").toInt();
    p.totalVotes = obj.value("total_votes").toInt();
    return p;
}

PollDetails ApiClient::parsePollDetails(const QJsonObject &obj)
{
    PollDetails p;
    p.pollId = obj.value("poll_id").toString();
    p.title = obj.value("title").toString();
    p.publicKey = obj.value("public_key").toString();
    p.createdAt = obj.value("created_at").toString();
    p.isOpen = obj.value("is_open").toBool();
    p.candidates = jsonArrayToStringList(obj.value("candidates").toArray());
    p.issuedTokens = obj.value("issued_tokens").toInt();
    p.signedTokens = obj.value("signed_tokens").toInt();
    p.totalVotes = obj.value("total_votes").toInt();
    p.results = obj.value("results").toArray();
    return p;
}

CreatePollResponse ApiClient::parseCreatePollResponse(const QJsonObject &obj)
{
    CreatePollResponse r;
    r.pollId = obj.value("poll_id").toString();
    r.title = obj.value("title").toString();
    r.publicKey = obj.value("public_key").toString();
    r.createdAt = obj.value("created_at").toString();
    r.isOpen = obj.value("is_open").toBool();
    r.candidates = jsonArrayToStringList(obj.value("candidates").toArray());
    r.issuedTokens = obj.value("issued_tokens").toInt();
    r.initialTokens = jsonArrayToStringList(obj.value("initial_tokens").toArray());
    return r;
}

IssueTokenResponse ApiClient::parseIssueTokenResponse(const QJsonObject &obj)
{
    IssueTokenResponse r;
    r.pollId = obj.value("poll_id").toString();
    r.count = obj.value("count").toInt();
    r.tokens = jsonArrayToStringList(obj.value("tokens").toArray());
    return r;
}

SignResponse ApiClient::parseSignResponse(const QJsonObject &obj)
{
    SignResponse r;
    r.pollId = obj.value("poll_id").toString();
    r.blindSignature = obj.value("blind_signature").toString();
    return r;
}

SubmitVoteResponse ApiClient::parseSubmitVoteResponse(const QJsonObject &obj)
{
    SubmitVoteResponse r;
    r.pollId = obj.value("poll_id").toString();
    r.status = obj.value("status").toString();
    r.candidate = obj.value("candidate").toString();
    r.totalVotes = obj.value("total_votes").toInt();
    return r;
}

ResultsResponse ApiClient::parseResultsResponse(const QJsonObject &obj)
{
    ResultsResponse r;
    r.pollId = obj.value("poll_id").toString();
    r.title = obj.value("title").toString();
    r.isOpen = obj.value("is_open").toBool();
    r.totalVotes = obj.value("total_votes").toInt();
    r.results = obj.value("results").toArray();
    return r;
}

ClosePollResponse ApiClient::parseClosePollResponse(const QJsonObject &obj)
{
    ClosePollResponse r;
    r.pollId = obj.value("poll_id").toString();
    r.status = obj.value("status").toString();
    return r;
}

#include "outbox_store.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "utils/time.h"

static QJsonObject voteToJson(const dto::EncryptedVote& v) {
    QJsonObject o;
    o["ballot_id"] = v.ballotId;
    o["ciphertext"] = v.ciphertextB64;
    o["signature"] = v.signature;
    o["client_ts_ms"] = QString::number(v.clientTsMs);
    return o;
}

static dto::EncryptedVote voteFromJson(const QJsonObject& o) {
    dto::EncryptedVote v;
    v.ballotId = o.value("ballot_id").toString();
    v.ciphertextB64 = o.value("ciphertext").toString();
    v.signature = o.value("signature").toString();
    v.clientTsMs = o.value("client_ts_ms").toString().toLongLong();
    return v;
}

static QJsonObject itemToJson(const OutboxItem& it) {
    QJsonObject o;
    o["id"] = it.id;
    o["poll_id"] = it.pollId;
    o["created_at_ms"] = QString::number(it.createdAtMs);
    o["submit_at_ms"] = QString::number(it.submitAtMs);
    o["attempts"] = it.attempts;
    o["last_error"] = it.lastError;
    o["vote"] = voteToJson(it.vote);
    return o;
}

static OutboxItem itemFromJson(const QJsonObject& o) {
    OutboxItem it;
    it.id = o.value("id").toString();
    it.pollId = o.value("poll_id").toString();
    it.createdAtMs = o.value("created_at_ms").toString().toLongLong();
    it.submitAtMs = o.value("submit_at_ms").toString().toLongLong();
    it.attempts = o.value("attempts").toInt();
    it.lastError = o.value("last_error").toString();
    it.vote = voteFromJson(o.value("vote").toObject());
    return it;
}

OutboxStore::OutboxStore(QObject* parent)
    : QObject(parent)
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    path_ = base + QDir::separator() + "outbox.json";

    load();
}

QString OutboxStore::storagePath() const { return path_; }

QVector<OutboxItem> OutboxStore::items() const { return items_; }

int OutboxStore::indexOf(const QString& id) const {
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == id) return i;
    }
    return -1;
}

void OutboxStore::upsert(const OutboxItem& item) {
    const int idx = indexOf(item.id);
    if (idx >= 0) items_[idx] = item;
    else items_.push_back(item);

    save();
    emit changed();
}

bool OutboxStore::remove(const QString& id) {
    const int idx = indexOf(id);
    if (idx < 0) return false;
    items_.removeAt(idx);
    save();
    emit changed();
    return true;
}

QVector<OutboxItem> OutboxStore::due(qint64 nowMs) const {
    QVector<OutboxItem> out;
    for (const auto& it : items_) {
        if (it.submitAtMs <= nowMs) out.push_back(it);
    }
    return out;
}

void OutboxStore::load() {
    items_.clear();

    QFile f(path_);
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly)) return;

    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;

    const auto arr = doc.array();
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        items_.push_back(itemFromJson(v.toObject()));
    }
}

void OutboxStore::save() const {
    QJsonArray arr;
    for (const auto& it : items_) arr.append(itemToJson(it));

    QFile f(path_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

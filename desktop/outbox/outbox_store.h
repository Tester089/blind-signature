#pragma once

#include <QObject>
#include <QVector>
#include <QString>

#include "net/dto.h"

struct OutboxItem {
    QString id;
    QString pollId;
    qint64 createdAtMs = 0;
    qint64 submitAtMs = 0;
    int attempts = 0;
    QString lastError;

    dto::EncryptedVote vote;
};

class OutboxStore : public QObject {
    Q_OBJECT
public:
    explicit OutboxStore(QObject* parent = nullptr);

    QString storagePath() const;

    QVector<OutboxItem> items() const;
    void upsert(const OutboxItem& item);
    bool remove(const QString& id);

    QVector<OutboxItem> due(qint64 nowMs) const;

    void load();
    void save() const;

signals:
    void changed();

private:
    QString path_;
    QVector<OutboxItem> items_;

    int indexOf(const QString& id) const;
};

#pragma once

#include <QObject>
#include <QSet>

class QTimer;
class ApiClient;
class OutboxStore;
struct OutboxItem;

class OutboxWorker : public QObject {
    Q_OBJECT
public:
    OutboxWorker(ApiClient* api, OutboxStore* store, QObject* parent = nullptr);

    void start();
    void stop();

    void setMaxAttempts(int v);
    int maxAttempts() const;

signals:
    void itemStatus(const QString& id, const QString& text, bool isError);

private:
    void tick();

    ApiClient* api_;
    OutboxStore* store_;
    QTimer* timer_;
    QSet<QString> inFlight_;
    int maxAttempts_ = 5;
};

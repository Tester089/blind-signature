#include "outbox_worker.h"

#include <QTimer>

#include "net/api_client.h"
#include "outbox_store.h"
#include "utils/time.h"

OutboxWorker::OutboxWorker(ApiClient* api, OutboxStore* store, QObject* parent)
    : QObject(parent), api_(api), store_(store), timer_(new QTimer(this))
{
    timer_->setInterval(1000);

    connect(timer_, &QTimer::timeout, this, &OutboxWorker::tick);

    connect(api_, &ApiClient::submitFinished, this,
            [this](const QString& requestId, const QString& pollId, const dto::SubmitResult& res) {
                Q_UNUSED(pollId);
                inFlight_.remove(requestId);

                // Find item and update/remove.
                const auto items = store_->items();
                for (const auto& it : items) {
                    if (it.id != requestId) continue;

                    if (res.accepted) {
                        store_->remove(it.id);
                        emit itemStatus(it.id, "Sent (receipt " + res.receiptId + ")", false);
                    } else {
                        OutboxItem upd = it;
                        upd.attempts += 1;
                        upd.lastError = res.error;

                        // simple backoff: 2^attempts seconds
                        const qint64 backoff = (1LL << qMin(upd.attempts, 10)) * 1000;
                        upd.submitAtMs = util::nowMs() + backoff;
                        store_->upsert(upd);

                        emit itemStatus(it.id, "Send failed: " + res.error + ", retry in " + util::formatMs(backoff), true);
                    }
                    break;
                }
            });
}

void OutboxWorker::start() { timer_->start(); }
void OutboxWorker::stop() { timer_->stop(); }

void OutboxWorker::setMaxAttempts(int v) { maxAttempts_ = v; }
int OutboxWorker::maxAttempts() const { return maxAttempts_; }

void OutboxWorker::tick() {
    const qint64 now = util::nowMs();
    const auto due = store_->due(now);

    for (const auto& it : due) {
        if (inFlight_.contains(it.id)) continue;
        if (it.attempts >= maxAttempts_) {
            emit itemStatus(it.id, "Dropped: max attempts reached", true);
            store_->remove(it.id);
            continue;
        }

        inFlight_.insert(it.id);
        emit itemStatus(it.id, "Sending...", false);
        api_->submitEncrypted(it.id, it.pollId, it.vote);
    }
}

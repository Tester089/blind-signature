#include "time.h"

#include <QDateTime>

namespace util {

qint64 nowMs() {
    return QDateTime::currentMSecsSinceEpoch();
}

QString formatMs(qint64 ms) {
    qint64 s = ms / 1000;
    const qint64 h = s / 3600; s %= 3600;
    const qint64 m = s / 60; s %= 60;

    if (h > 0) return QString("%1h %2m %3s").arg(h).arg(m).arg(s);
    if (m > 0) return QString("%1m %2s").arg(m).arg(s);
    return QString("%1s").arg(s);
}

QString formatEta(qint64 targetMs) {
    const qint64 d = targetMs - nowMs();
    if (d <= 0) return "now";
    return formatMs(d);
}

}

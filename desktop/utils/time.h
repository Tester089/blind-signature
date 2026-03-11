#pragma once

#include <QString>
#include <QtGlobal>

namespace util {

qint64 nowMs();
QString formatMs(qint64 ms);
QString formatEta(qint64 targetMs);

}

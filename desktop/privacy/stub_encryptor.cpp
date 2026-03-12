#include "stub_encryptor.h"

#include <QByteArray>

QString StubEncryptor::encryptToBase64(const QString& plain, const QString& encPublicKey) {
    Q_UNUSED(encPublicKey);

    return plain.toUtf8().toBase64();
}

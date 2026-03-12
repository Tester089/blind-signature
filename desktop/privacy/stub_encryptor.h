#pragma once

#include "encryptor.h"

class StubEncryptor final : public IEncryptor {
public:
    QString encryptToBase64(const QString& plain, const QString& encPublicKey) override;
};

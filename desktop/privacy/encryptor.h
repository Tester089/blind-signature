#pragma once

#include <QString>

class IEncryptor {
public:
    virtual ~IEncryptor() = default;

    // Returns BASE64 ciphertext.
    virtual QString encryptToBase64(const QString& plain, const QString& encPublicKey) = 0;
};

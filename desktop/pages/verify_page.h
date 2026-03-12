#pragma once

#include <QWidget>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;

class VerifyPage : public QWidget {
    Q_OBJECT
public:
    explicit VerifyPage(QWidget* parent = nullptr);

    void setPublicKey(const QString& pk);
    void setMessage(const QString& msg);
    void setSignature(const QString& sig);

private:
    void doVerify();

    QTextEdit* message_;
    QLineEdit* publicKey_;
    QLineEdit* signature_;
    QPushButton* verifyBtn_;
    QLabel* result_;
};

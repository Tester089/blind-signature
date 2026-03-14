#include "verify_page.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "widgets/card_frame.h"
#include "core.h"

VerifyPage::VerifyPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* card = new CardFrame("Verify signature", this);

    message_ = new QTextEdit(this);
    message_->setPlaceholderText("Message (or ballot_id|ciphertext)");
    message_->setMinimumHeight(90);
    message_->setStyleSheet("QTextEdit{border-radius:12px; padding:8px; background:#111116; border:1px solid #2e2e3a;}");

    publicKey_ = new QLineEdit(this);
    publicKey_->setPlaceholderText("Public key");
    publicKey_->setStyleSheet("QLineEdit{border-radius:10px; padding:8px; background:#111116; border:1px solid #2e2e3a;}");

    signature_ = new QLineEdit(this);
    signature_->setPlaceholderText("Signature");
    signature_->setStyleSheet("QLineEdit{border-radius:10px; padding:8px; background:#111116; border:1px solid #2e2e3a;}");

    auto* form = new QFormLayout();
    form->addRow("Message:", message_);
    form->addRow("Public key:", publicKey_);
    form->addRow("Signature:", signature_);

    verifyBtn_ = new QPushButton("Verify", this);
    verifyBtn_->setStyleSheet("QPushButton{border-radius:12px; padding:10px 14px; background:#2a2a35; border:1px solid #3a3a48;} QPushButton:hover{background:#32323f;}");

    result_ = new QLabel("No verification yet", this);
    result_->setStyleSheet("padding:10px 12px; border-radius:12px; background:#2a2a35; border:1px solid #3a3a48; font-weight:700;");

    card->bodyLayout()->addLayout(form);
    card->bodyLayout()->addWidget(verifyBtn_);
    card->bodyLayout()->addWidget(result_);

    root->addWidget(card);
    root->addStretch(1);

    connect(verifyBtn_, &QPushButton::clicked, this, &VerifyPage::doVerify);
}

void VerifyPage::setPublicKey(const QString& pk) { publicKey_->setText(pk); }
void VerifyPage::setMessage(const QString& msg) { message_->setPlainText(msg); }
void VerifyPage::setSignature(const QString& sig) { signature_->setText(sig); }

void VerifyPage::doVerify() {
    const QString pk = publicKey_->text().trimmed();
    const QString msg = message_->toPlainText().trimmed();
    const QString sig = signature_->text().trimmed();

    if (pk.isEmpty() || msg.isEmpty() || sig.isEmpty()) {
        result_->setText("Fill all fields");
        result_->setStyleSheet("padding:10px 12px; border-radius:12px; background:#5a1f1f; border:1px solid #6d2a2a; font-weight:800; color:#ffd1d1;");
        return;
    }

    const bool ok = Verify(pk.toStdString(), msg.toStdString(), sig.toStdString());

    if (ok) {
        result_->setText("VALID");
        result_->setStyleSheet("padding:10px 12px; border-radius:12px; background:#1f4d2e; border:1px solid #2a6a41; font-weight:900; color:#d8ffe5;");
    } else {
        result_->setText("INVALID");
        result_->setStyleSheet("padding:10px 12px; border-radius:12px; background:#5a1f1f; border:1px solid #6d2a2a; font-weight:900; color:#ffd1d1;");
    }
}

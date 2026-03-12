#include "card_frame.h"

#include <QLabel>
#include <QVBoxLayout>

CardFrame::CardFrame(QString title, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("CardFrame");
    setFrameShape(QFrame::NoFrame);

    setStyleSheet(
        "#CardFrame {"
        "  background: #1a1a22;"
        "  border: 1px solid #2e2e3a;"
        "  border-radius: 14px;"
        "}"
        "#CardTitle {"
        "  color: #eaeaf2;"
        "  font-weight: 700;"
        "  font-size: 13px;"
        "}"
    );

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(10);

    title_ = new QLabel(std::move(title), this);
    title_->setObjectName("CardTitle");

    body_ = new QVBoxLayout();
    body_->setSpacing(8);

    root->addWidget(title_);
    root->addLayout(body_);
}

void CardFrame::setTitle(const QString& t) {
    title_->setText(t);
}

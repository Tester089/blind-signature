#include "copy_field.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>

CopyField::CopyField(QWidget* parent) : QWidget(parent) {
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(8);

    edit_ = new QLineEdit(this);
    edit_->setReadOnly(true);
    edit_->setClearButtonEnabled(true);
    edit_->setStyleSheet("QLineEdit{border-radius:10px; padding:8px; background:#111116; border:1px solid #2e2e3a;}");

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(mono.pointSize() - 1);
    edit_->setFont(mono);

    copyBtn_ = new QToolButton(this);
    copyBtn_->setText("Copy");
    copyBtn_->setStyleSheet("QToolButton{border-radius:10px; padding:8px 12px; background:#2a2a35; border:1px solid #3a3a48;} QToolButton:hover{background:#32323f;}");

    connect(copyBtn_, &QToolButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(edit_->text());
    });

    l->addWidget(edit_, 1);
    l->addWidget(copyBtn_);
}

void CopyField::setText(const QString& t) { edit_->setText(t); }
QString CopyField::text() const { return edit_->text(); }

#include "settings_page.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

#include "widgets/card_frame.h"

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* card = new CardFrame("Connection & privacy", this);

    serverUrl_ = new QLineEdit(this);
    serverUrl_->setPlaceholderText("http://127.0.0.1:8080");
    serverUrl_->setStyleSheet("QLineEdit{border-radius:10px; padding:8px; background:#111116; border:1px solid #2e2e3a;}");

    delayEnabled_ = new QCheckBox("Delay sending (privacy)", this);
    delayEnabled_->setChecked(true);

    delayMin_ = new QSpinBox(this);
    delayMax_ = new QSpinBox(this);
    delayMin_->setRange(0, 3600);
    delayMax_->setRange(0, 3600);
    delayMin_->setValue(10);
    delayMax_->setValue(30);

    hint_ = new QLabel("Tip: delay helps against timing correlation.", this);
    hint_->setStyleSheet("color:#b9b9c6;");

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->addRow("Server URL:", serverUrl_);
    form->addRow("", delayEnabled_);
    form->addRow("Min delay (sec):", delayMin_);
    form->addRow("Max delay (sec):", delayMax_);

    card->bodyLayout()->addLayout(form);
    card->bodyLayout()->addWidget(hint_);

    apply_ = new QPushButton("Apply", this);
    apply_->setStyleSheet("QPushButton{border-radius:12px; padding:10px 14px; background:#2a2a35; border:1px solid #3a3a48;} QPushButton:hover{background:#32323f;}");

    root->addWidget(card);
    root->addStretch(1);
    root->addWidget(apply_, 0, Qt::AlignRight);

    connect(apply_, &QPushButton::clicked, this, [this]() { emitChanged(); });
    connect(serverUrl_, &QLineEdit::editingFinished, this, [this]() { emitChanged(); });
    connect(delayEnabled_, &QCheckBox::toggled, this, [this]() { emitChanged(); });
    connect(delayMin_, qOverload<int>(&QSpinBox::valueChanged), this, [this]() { emitChanged(); });
    connect(delayMax_, qOverload<int>(&QSpinBox::valueChanged), this, [this]() { emitChanged(); });


    serverUrl_->setText("http://127.0.0.1:8080");
}

UiSettings SettingsPage::settings() const {
    UiSettings s;
    s.serverUrl = serverUrl_->text().trimmed();
    s.delayEnabled = delayEnabled_->isChecked();
    s.delayMinSec = delayMin_->value();
    s.delayMaxSec = delayMax_->value();
    if (s.delayMaxSec < s.delayMinSec) s.delayMaxSec = s.delayMinSec;
    return s;
}

void SettingsPage::setSettings(const UiSettings& s) {
    serverUrl_->setText(s.serverUrl);
    delayEnabled_->setChecked(s.delayEnabled);
    delayMin_->setValue(s.delayMinSec);
    delayMax_->setValue(s.delayMaxSec);
}

void SettingsPage::emitChanged() {
    emit settingsChanged(settings());
}

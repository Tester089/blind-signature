#include "settings_page.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget* parent)
        : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto* titleLabel = new QLabel("Settings");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 700;");

    statusLabel_ = new QLabel("Status: Settings are not saved yet");
    statusLabel_->setStyleSheet("padding: 8px 12px; border-radius: 8px; background: #2d2d2d;");

    rootLayout->addWidget(titleLabel);
    rootLayout->addWidget(statusLabel_);

    auto* box = new QGroupBox("Application settings");
    auto* form = new QFormLayout(box);

    serverUrlEdit_ = new QLineEdit;
    serverUrlEdit_->setPlaceholderText("http://127.0.0.1:8080");

    darkThemeCheck_ = new QCheckBox("Enable dark theme");
    darkThemeCheck_->setChecked(true);

    fakeDelayCheck_ = new QCheckBox("Use fake delay for stub/demo");
    fakeDelayCheck_->setChecked(true);

    form->addRow("Server URL:", serverUrlEdit_);
    form->addRow("", darkThemeCheck_);
    form->addRow("", fakeDelayCheck_);

    rootLayout->addWidget(box);

    auto* buttonsLayout = new QHBoxLayout;
    saveButton_ = new QPushButton("Save");
    resetButton_ = new QPushButton("Reset demo values");

    buttonsLayout->addWidget(saveButton_);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(resetButton_);

    rootLayout->addLayout(buttonsLayout);
    rootLayout->addStretch();

    connect(saveButton_, &QPushButton::clicked, this, [this]() {
        if (serverUrl().trimmed().isEmpty()) {
            setStatus("Status: Server URL is empty", true);
            return;
        }
        emit saveRequested(serverUrl(), darkThemeEnabled(), fakeDelayEnabled());
    });

    connect(resetButton_, &QPushButton::clicked, this, [this]() {
        serverUrlEdit_->setText("http://127.0.0.1:8080");
        darkThemeCheck_->setChecked(true);
        fakeDelayCheck_->setChecked(true);
        setStatus("Status: Demo settings restored");
    });
}

QString SettingsPage::serverUrl() const
{
    return serverUrlEdit_->text();
}

bool SettingsPage::darkThemeEnabled() const
{
    return darkThemeCheck_->isChecked();
}

bool SettingsPage::fakeDelayEnabled() const
{
    return fakeDelayCheck_->isChecked();
}

void SettingsPage::setServerUrl(const QString& url)
{
    serverUrlEdit_->setText(url);
}

void SettingsPage::setStatus(const QString& text, bool isError)
{
    statusLabel_->setText(text);
    if (isError) {
        statusLabel_->setStyleSheet(
                "padding: 8px 12px; border-radius: 8px; background: #5a1f1f; color: #ffb3b3;");
    } else {
        statusLabel_->setStyleSheet(
                "padding: 8px 12px; border-radius: 8px; background: #2d2d2d;");
    }
}
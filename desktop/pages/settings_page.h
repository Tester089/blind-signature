#pragma once

#include <QWidget>

class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;

class SettingsPage : public QWidget {
Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    QString serverUrl() const;
    QString username() const;
    QString password() const;
    bool darkThemeEnabled() const;
    bool fakeDelayEnabled() const;

    void setServerUrl(const QString& url);
    void setUsername(const QString& username);
    void setPassword(const QString& password);
    void setStatus(const QString& text, bool isError = false);

signals:
    void saveRequested(const QString& serverUrl,
                       const QString& username,
                       const QString& password,
                       bool darkTheme,
                       bool fakeDelay);

private:
    QLineEdit* serverUrlEdit_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QCheckBox* darkThemeCheck_;
    QCheckBox* fakeDelayCheck_;
    QLabel* statusLabel_;
    QPushButton* saveButton_;
    QPushButton* resetButton_;
};
#pragma once

#include <QWidget>

class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;

class SettingsPage : public QWidget
{
Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    QString serverUrl() const;
    bool darkThemeEnabled() const;
    bool fakeDelayEnabled() const;

    void setServerUrl(const QString& url);
    void setStatus(const QString& text, bool isError = false);

signals:
    void saveRequested(const QString& serverUrl, bool darkTheme, bool fakeDelay);

private:
    QLineEdit* serverUrlEdit_;
    QCheckBox* darkThemeCheck_;
    QCheckBox* fakeDelayCheck_;
    QLabel* statusLabel_;
    QPushButton* saveButton_;
    QPushButton* resetButton_;
};
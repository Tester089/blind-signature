#pragma once

#include <QWidget>

struct UiSettings {
    QString serverUrl;
    bool delayEnabled = true;
    int delayMinSec = 10;
    int delayMaxSec = 30;
};

class QLineEdit;
class QCheckBox;
class QSpinBox;
class QLabel;
class QPushButton;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);

    UiSettings settings() const;
    void setSettings(const UiSettings& s);

signals:
    void settingsChanged(const UiSettings& s);

private:
    void emitChanged();

    QLineEdit* serverUrl_;
    QCheckBox* delayEnabled_;
    QSpinBox* delayMin_;
    QSpinBox* delayMax_;
    QLabel* hint_;
    QPushButton* apply_;
};

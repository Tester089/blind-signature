#pragma once

#include <QMainWindow>

class QTabWidget;
class DashboardPage;
class VerifyPage;
class SettingsPage;

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupUi();
    void setupConnections();

    QTabWidget* tabs_;
    DashboardPage* dashboardPage_;
    VerifyPage* verifyPage_;
    SettingsPage* settingsPage_;
};
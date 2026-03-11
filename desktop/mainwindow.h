#pragma once

#include <QMainWindow>

class QTabWidget;

class ApiClient;
class IEncryptor;
class OutboxStore;
class OutboxWorker;

class DashboardPage;
class VerifyPage;
class SettingsPage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    QTabWidget* tabs_;

    ApiClient* api_;
    IEncryptor* encryptor_;
    OutboxStore* outbox_;
    OutboxWorker* worker_;

    DashboardPage* dashboard_;
    VerifyPage* verify_;
    SettingsPage* settings_;
};

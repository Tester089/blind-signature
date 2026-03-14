#include "mainwindow.h"

#include <QSettings>
#include <QTabWidget>

#include "pages/dashboard_page.h"
#include "pages/verify_page.h"
#include "pages/settings_page.h"

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          tabs_(nullptr),
          dashboardPage_(nullptr),
          verifyPage_(nullptr),
          settingsPage_(nullptr) {
    setupUi();
    setupConnections();
    setWindowTitle("Blind Signature Desktop");
    resize(1280, 820);
}

void MainWindow::setupUi() {
    tabs_ = new QTabWidget(this);

    dashboardPage_ = new DashboardPage(this);
    verifyPage_ = new VerifyPage(this);
    settingsPage_ = new SettingsPage(this);

    tabs_->addTab(dashboardPage_, "Dashboard");
    tabs_->addTab(verifyPage_, "Verify");
    tabs_->addTab(settingsPage_, "Settings");

    setCentralWidget(tabs_);

    QSettings settings;
    const QString serverUrl = settings.value("server/url", "http://127.0.0.1:8080/").toString();
    const QString username = settings.value("user/username", "").toString();
    const QString password = settings.value("user/password", "").toString();

    settingsPage_->setServerUrl(serverUrl);
    settingsPage_->setUsername(username);
    settingsPage_->setPassword(password);

    dashboardPage_->setServerUrl(serverUrl);
    dashboardPage_->setUsername(username);
    dashboardPage_->setPassword(password);
}

void MainWindow::setupConnections() {
    connect(settingsPage_,
            &SettingsPage::saveRequested,
            this,
            [this](const QString& serverUrl,
                   const QString& username,
                   const QString& password,
                   bool darkTheme,
                   bool fakeDelay) {
                Q_UNUSED(darkTheme);
                Q_UNUSED(fakeDelay);

                QSettings settings;
                settings.setValue("server/url", serverUrl);
                settings.setValue("user/username", username);
                settings.setValue("user/password", password);

                dashboardPage_->setServerUrl(serverUrl);
                dashboardPage_->setUsername(username);
                dashboardPage_->setPassword(password);

                settingsPage_->setStatus("Status: Settings saved");
            });
}
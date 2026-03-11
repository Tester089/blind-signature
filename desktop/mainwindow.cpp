#include "mainwindow.h"

#include <QTabWidget>

#include "net/api_client.h"
#include "privacy/stub_encryptor.h"
#include "outbox/outbox_store.h"
#include "outbox/outbox_worker.h"

#include "pages/dashboard_page.h"
#include "pages/verify_page.h"
#include "pages/settings_page.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , tabs_(new QTabWidget(this))
    , api_(new ApiClient(this))
    , encryptor_(new StubEncryptor())
    , outbox_(new OutboxStore(this))
    , worker_(new OutboxWorker(api_, outbox_, this))
    , dashboard_(new DashboardPage(api_, encryptor_, outbox_, worker_, this))
    , verify_(new VerifyPage(this))
    , settings_(new SettingsPage(this))
{
    setWindowTitle("Blind Signature");
    resize(1120, 720);

    tabs_->addTab(dashboard_, "Dashboard");
    tabs_->addTab(verify_, "Verify");
    tabs_->addTab(settings_, "Settings");

    setCentralWidget(tabs_);

    // Wire settings -> dashboard/api
    connect(settings_, &SettingsPage::settingsChanged, dashboard_, &DashboardPage::applySettings);

    // Wire last vote -> verify
    connect(dashboard_, &DashboardPage::lastVoteForVerify, this,
            [this](const QString& pk, const QString& msg, const QString& sig) {
                verify_->setPublicKey(pk);
                verify_->setMessage(msg);
                verify_->setSignature(sig);
            });

    // Apply defaults from settings page once
    dashboard_->applySettings(settings_->settings());

    worker_->start();
}

MainWindow::~MainWindow() {
    delete encryptor_;
}

#include "dashboard_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QRandomGenerator>
#include <QDateTime>
#include <QUuid>

#include "widgets/connect_button.h"
#include "widgets/status_badge.h"
#include "widgets/card_frame.h"
#include "widgets/copy_field.h"

#include "net/api_client.h"
#include "privacy/encryptor.h"
#include "outbox/outbox_store.h"
#include "outbox/outbox_worker.h"
#include "utils/time.h"

#include "core/core.h"

static QString chipStyle() {
    return "QLabel{padding:6px 10px; border-radius:12px; background:#2a2a35; border:1px solid #3a3a48; color:#eaeaf2; font-weight:700;}";
}

class CreatePollDialog final : public QDialog {
public:
    explicit CreatePollDialog(QWidget* parent=nullptr) : QDialog(parent) {
        setWindowTitle("Create poll");
        setModal(true);
        resize(520, 360);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(14, 14, 14, 14);
        root->setSpacing(10);

        title_ = new QLineEdit(this);
        title_->setPlaceholderText("Poll title");
        title_->setText("New poll");

        candidates_ = new QTextEdit(this);
        candidates_->setPlaceholderText("Candidates (one per line)\nAlice\nBob\nCarol");
        candidates_->setPlainText("Alice\nBob\nCarol");

        tokenCount_ = new QSpinBox(this);
        tokenCount_->setRange(1, 1000);
        tokenCount_->setValue(5);

        encryption_ = new QCheckBox("Enable encryption (recommended)", this);
        encryption_->setChecked(true);

        auto* form = new QGridLayout();
        form->addWidget(new QLabel("Title:"), 0, 0);
        form->addWidget(title_, 0, 1);
        form->addWidget(new QLabel("Tokens:"), 1, 0);
        form->addWidget(tokenCount_, 1, 1);
        form->addWidget(encryption_, 2, 1);
        form->addWidget(new QLabel("Candidates:"), 3, 0);
        form->addWidget(candidates_, 3, 1);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        root->addLayout(form);
        root->addWidget(buttons);

        setStyleSheet(
            "QDialog{background:#141418; color:#f0f0f0;}"
            "QLineEdit,QTextEdit{border-radius:10px; padding:8px; background:#111116; border:1px solid #2e2e3a;}"
            "QSpinBox{border-radius:10px; padding:6px; background:#111116; border:1px solid #2e2e3a;}"
            "QPushButton{border-radius:10px; padding:8px 12px; background:#2a2a35; border:1px solid #3a3a48;}"
            "QPushButton:hover{background:#32323f;}"
        );
    }

    dto::CreatePollRequest request() const {
        dto::CreatePollRequest r;
        r.title = title_->text().trimmed();
        r.tokenCount = tokenCount_->value();
        r.encryptionEnabled = encryption_->isChecked();

        const QStringList lines = candidates_->toPlainText().split('\n');
        for (const auto& s : lines) {
            const QString t = s.trimmed();
            if (!t.isEmpty()) r.candidates.push_back(t);
        }
        if (r.candidates.isEmpty()) r.candidates = {"Alice", "Bob"};
        if (r.title.isEmpty()) r.title = "New poll";
        return r;
    }

private:
    QLineEdit* title_;
    QTextEdit* candidates_;
    QSpinBox* tokenCount_;
    QCheckBox* encryption_;
};

DashboardPage::DashboardPage(ApiClient* api, IEncryptor* encryptor, OutboxStore* outbox, OutboxWorker* worker, QWidget* parent)
    : QWidget(parent), api_(api), encryptor_(encryptor), outbox_(outbox), worker_(worker)
{
    settings_.serverUrl = api_->baseUrl();

    setupUi();
    connectSignals();

    refreshPolls();
    updateOutboxInfo();
}

void DashboardPage::applySettings(const UiSettings& s) {
    settings_ = s;
    api_->setBaseUrl(s.serverUrl);
    serverChip_->setText("Server: " + s.serverUrl);

    log("Settings updated: delay=" + QString(s.delayEnabled ? "ON" : "OFF") +
        " [" + QString::number(s.delayMinSec) + ".." + QString::number(s.delayMaxSec) + " sec]");
}

void DashboardPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Header
    auto* header = new QHBoxLayout();
    header->setSpacing(10);

    auto* title = new QLabel("Blind Signature", this);
    title->setStyleSheet("font-size:18px; font-weight:900; color:#f0f0f6;");

    serverChip_ = new QLabel("Server: " + api_->baseUrl(), this);
    serverChip_->setStyleSheet(chipStyle());

    pollCombo_ = new QComboBox(this);
    pollCombo_->setMinimumWidth(260);
    pollCombo_->setStyleSheet("QComboBox{border-radius:12px; padding:6px 10px; background:#1a1a22; border:1px solid #2e2e3a;} QComboBox::drop-down{border:0;}");

    refreshBtn_ = new QPushButton("Refresh", this);
    refreshBtn_->setStyleSheet("QPushButton{border-radius:12px; padding:8px 12px; background:#2a2a35; border:1px solid #3a3a48;} QPushButton:hover{background:#32323f;}");

    createBtn_ = new QPushButton("Create poll…", this);
    createBtn_->setStyleSheet("QPushButton{border-radius:12px; padding:8px 12px; background:#2a2a35; border:1px solid #3a3a48;} QPushButton:hover{background:#32323f;}");

    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(serverChip_);
    header->addWidget(pollCombo_);
    header->addWidget(refreshBtn_);
    header->addWidget(createBtn_);

    // Main area
    auto* mainRow = new QHBoxLayout();
    mainRow->setSpacing(12);

    // Left: connect
    auto* leftCol = new QVBoxLayout();
    leftCol->setSpacing(12);

    connectBtn_ = new ConnectButton(this);
    connectBtn_->setCenterText("CAST\nVOTE");
    connectBtn_->setState(ConnectButton::State::Disconnected);
    connectBtn_->setProgress(0.0);

    statusBadge_ = new StatusBadge(this);
    statusBadge_->set(StatusBadge::State::Idle, "Select poll to start");

    outboxInfo_ = new QLabel("Outbox: 0 pending", this);
    outboxInfo_->setStyleSheet(chipStyle());

    leftCol->addWidget(connectBtn_, 0, Qt::AlignHCenter);
    leftCol->addWidget(statusBadge_);
    leftCol->addWidget(outboxInfo_, 0, Qt::AlignLeft);
    leftCol->addStretch(1);

    // Right: cards
    auto* rightCol = new QVBoxLayout();
    rightCol->setSpacing(12);

    auto* pollCard = new CardFrame("Poll", this);
    pollInfo_ = new QLabel("No poll selected", this);
    pollInfo_->setStyleSheet("color:#c7c7d6;");
    pollCard->bodyLayout()->addWidget(pollInfo_);

    auto* voteCard = new CardFrame("Vote", this);
    candidateCombo_ = new QComboBox(this);
    candidateCombo_->setStyleSheet("QComboBox{border-radius:12px; padding:8px 10px; background:#111116; border:1px solid #2e2e3a;} QComboBox::drop-down{border:0;}");
    candidateCombo_->addItem("—");
    voteCard->bodyLayout()->addWidget(new QLabel("Candidate:"));
    voteCard->bodyLayout()->addWidget(candidateCombo_);

    auto* privacyCard = new CardFrame("Privacy", this);
    auto* delayHint = new QLabel("Delay sending hides timing. Payload is stored locally in Outbox.", this);
    delayHint->setStyleSheet("color:#b9b9c6;");
    privacyCard->bodyLayout()->addWidget(delayHint);

    advancedToggle_ = new QPushButton("Advanced ▼", this);
    advancedToggle_->setCheckable(true);
    advancedToggle_->setChecked(false);
    advancedToggle_->setStyleSheet("QPushButton{border-radius:12px; padding:10px 14px; background:#2a2a35; border:1px solid #3a3a48;} QPushButton:hover{background:#32323f;}");

    advancedPanel_ = new QWidget(this);
    auto* advLayout = new QVBoxLayout(advancedPanel_);
    advLayout->setContentsMargins(0,0,0,0);
    advLayout->setSpacing(10);

    auto* protoCard = new CardFrame("Protocol data", this);
    msgField_ = new CopyField(this);
    blindedField_ = new CopyField(this);
    invField_ = new CopyField(this);
    blindSigField_ = new CopyField(this);
    finalSigField_ = new CopyField(this);
    cipherField_ = new CopyField(this);

    protoCard->bodyLayout()->addWidget(new QLabel("message_to_sign (ballot_id|ciphertext):"));
    protoCard->bodyLayout()->addWidget(msgField_);
    protoCard->bodyLayout()->addWidget(new QLabel("blinded_message:"));
    protoCard->bodyLayout()->addWidget(blindedField_);
    protoCard->bodyLayout()->addWidget(new QLabel("inverse:"));
    protoCard->bodyLayout()->addWidget(invField_);
    protoCard->bodyLayout()->addWidget(new QLabel("blind_signature:"));
    protoCard->bodyLayout()->addWidget(blindSigField_);
    protoCard->bodyLayout()->addWidget(new QLabel("final_signature:"));
    protoCard->bodyLayout()->addWidget(finalSigField_);
    protoCard->bodyLayout()->addWidget(new QLabel("ciphertext (base64):"));
    protoCard->bodyLayout()->addWidget(cipherField_);

    auto* logCard = new CardFrame("Log", this);
    logEdit_ = new QPlainTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setMaximumBlockCount(800);
    logEdit_->setStyleSheet("QPlainTextEdit{border-radius:12px; padding:10px; background:#0f0f14; border:1px solid #2e2e3a; font-family:Consolas,monospace; font-size:12px;}");
    logCard->bodyLayout()->addWidget(logEdit_);

    advLayout->addWidget(protoCard);
    advLayout->addWidget(logCard);

    advancedPanel_->setVisible(false);

    rightCol->addWidget(pollCard);
    rightCol->addWidget(voteCard);
    rightCol->addWidget(privacyCard);
    rightCol->addWidget(advancedToggle_);
    rightCol->addWidget(advancedPanel_);
    rightCol->addStretch(1);

    mainRow->addLayout(leftCol, 0);
    mainRow->addLayout(rightCol, 1);

    root->addLayout(header);
    root->addLayout(mainRow, 1);

    connectBtn_->setEnabled(false);
}

void DashboardPage::connectSignals() {
    connect(refreshBtn_, &QPushButton::clicked, this, &DashboardPage::refreshPolls);

    connect(createBtn_, &QPushButton::clicked, this, [this]() {
        CreatePollDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            const auto req = dlg.request();
            log("Creating poll: " + req.title);
            api_->createPoll(req);
        }
    });

    connect(pollCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &DashboardPage::onPollSelected);

    connect(advancedToggle_, &QPushButton::toggled, this, [this](bool on) {
        advancedPanel_->setVisible(on);
        advancedToggle_->setText(on ? "Advanced ▲" : "Advanced ▼");
    });

    connect(connectBtn_, &ConnectButton::clicked, this, &DashboardPage::startVote);

    connect(api_, &ApiClient::pollsRefreshed, this, [this](const QVector<dto::PollSummary>& polls) {
        const QString prev = poll_.pollId;
        pollCombo_->blockSignals(true);
        pollCombo_->clear();
        pollCombo_->addItem("Select poll…", "");
        for (const auto& p : polls) {
            pollCombo_->addItem(p.title + (p.isOpen ? "" : " (closed)"), p.pollId);
        }
        pollCombo_->blockSignals(false);

        // try to restore selection
        if (!prev.isEmpty()) {
            for (int i = 0; i < pollCombo_->count(); ++i) {
                if (pollCombo_->itemData(i).toString() == prev) {
                    pollCombo_->setCurrentIndex(i);
                    break;
                }
            }
        }

        log("Poll list updated: " + QString::number(polls.size()));
    });

    connect(api_, &ApiClient::pollFetched, this, [this](const dto::PollDetails& d) {
        poll_ = d;
        tokens_.clear();

        pollInfo_->setText(QString("%1\nID: %2\nStatus: %3\nCandidates: %4\nEncryption: %5")
                              .arg(d.title)
                              .arg(d.pollId)
                              .arg(d.isOpen ? "OPEN" : "CLOSED")
                              .arg(d.candidates.size())
                              .arg(d.encryptionEnabled ? "ON" : "OFF"));

        candidateCombo_->clear();
        for (const auto& c : d.candidates) candidateCombo_->addItem(c);

        connectBtn_->setEnabled(d.isOpen && !d.candidates.isEmpty());
        connectBtn_->setState(d.isOpen ? ConnectButton::State::Ready : ConnectButton::State::Disconnected);
        connectBtn_->setProgress(0.0);

        statusBadge_->set(StatusBadge::State::Idle, d.isOpen ? "Ready" : "Poll is closed");
        log("Selected poll: " + d.pollId);
    });

    connect(api_, &ApiClient::pollCreated, this, [this](const dto::CreatePollResponse& resp) {
        log("Poll created: " + resp.poll.pollId);
        tokens_ = resp.initialTokens;

        refreshPolls();
        // select the created poll
        api_->fetchPoll(resp.poll.pollId);
    });

    connect(api_, &ApiClient::tokensIssued, this, [this](const QString& pollId, const dto::IssueTokenResponse& resp) {
        if (pollId != poll_.pollId) return;
        for (const auto& t : resp.tokens) tokens_.push_back(t);
        log("Issued tokens: +" + QString::number(resp.tokens.size()));

        continueAfterToken();
    });

    connect(api_, &ApiClient::blindSigned, this, [this](const QString& pollId, const dto::BlindSignResponse& resp) {
        if (pollId != poll_.pollId) return;
        blindSig_ = resp.blindSignature;
        blindSigField_->setText(blindSig_);

        // Finalize
        finalSig_ = QString::fromStdString(Finalize(poll_.signPublicKey.toStdString(), messageToSign_.toStdString(), blindSig_.toStdString(), inverse_.toStdString()));
        finalSigField_->setText(finalSig_);

        connectBtn_->setProgress(0.78);
        statusBadge_->set(StatusBadge::State::Busy, "Encrypt & schedule");

        scheduleOrSend();
    });

    connect(api_, &ApiClient::networkError, this, [this](const QString& msg) {
        log("ERROR: " + msg);
        statusBadge_->set(StatusBadge::State::Error, msg);
        connectBtn_->setState(ConnectButton::State::Error);
    });

    connect(outbox_, &OutboxStore::changed, this, &DashboardPage::updateOutboxInfo);

    connect(worker_, &OutboxWorker::itemStatus, this, [this](const QString& id, const QString& text, bool isError) {
        log(QString("Outbox[%1]: %2").arg(id, text));
        if (!outboxIdLast_.isEmpty() && id == outboxIdLast_) {
            statusBadge_->set(isError ? StatusBadge::State::Error : StatusBadge::State::Ok, text);
            connectBtn_->setState(isError ? ConnectButton::State::Error : ConnectButton::State::Sent);
            connectBtn_->setProgress(isError ? 1.0 : 1.0);
        }
    });
}

void DashboardPage::refreshPolls() {
    api_->refreshPolls();
}

void DashboardPage::onPollSelected(int idx) {
    const QString id = pollCombo_->itemData(idx).toString();
    if (id.isEmpty()) {
        poll_ = {};
        pollInfo_->setText("No poll selected");
        candidateCombo_->clear();
        connectBtn_->setEnabled(false);
        connectBtn_->setState(ConnectButton::State::Disconnected);
        connectBtn_->setProgress(0.0);
        statusBadge_->set(StatusBadge::State::Idle, "Select poll to start");
        return;
    }
    api_->fetchPoll(id);
}

void DashboardPage::startVote() {
    if (poll_.pollId.isEmpty()) {
        statusBadge_->set(StatusBadge::State::Error, "Select poll first");
        return;
    }
    if (!poll_.isOpen) {
        statusBadge_->set(StatusBadge::State::Error, "Poll is closed");
        return;
    }

    candidate_ = candidateCombo_->currentText().trimmed();
    if (candidate_.isEmpty()) {
        statusBadge_->set(StatusBadge::State::Error, "Select candidate");
        return;
    }

    connectBtn_->setState(ConnectButton::State::Working);
    connectBtn_->setProgress(0.08);
    statusBadge_->set(StatusBadge::State::Busy, "Issuing token");

    log("== CAST VOTE ==");

    // Ensure token
    if (tokens_.isEmpty()) {
        api_->issueToken(poll_.pollId, 1);
        return;
    }

    continueAfterToken();
}

void DashboardPage::continueAfterToken() {
    if (tokens_.isEmpty()) {
        statusBadge_->set(StatusBadge::State::Error, "No token available");
        connectBtn_->setState(ConnectButton::State::Error);
        return;
    }

    const QString token = tokens_.front();
    tokens_.pop_front();

    connectBtn_->setProgress(0.18);
    statusBadge_->set(StatusBadge::State::Busy, "Preparing ballot");

    ballotId_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ballotPlain_ = candidate_ + "|" + ballotId_;


    ciphertext_ = encryptor_->encryptToBase64(ballotPlain_, poll_.encPublicKey);


    messageToSign_ = ballotId_ + "|" + ciphertext_;

    msgField_->setText(messageToSign_);
    cipherField_->setText(ciphertext_);

    connectBtn_->setProgress(0.32);
    statusBadge_->set(StatusBadge::State::Busy, "Blinding");

    // Blind
    const std::string res = Blind(poll_.signPublicKey.toStdString(), messageToSign_.toStdString());
    const auto pos = res.find(':');
    blinded_ = QString::fromStdString(pos == std::string::npos ? res : res.substr(0, pos));
    inverse_ = QString::fromStdString(pos == std::string::npos ? "" : res.substr(pos + 1));

    blindedField_->setText(blinded_);
    invField_->setText(inverse_);

    connectBtn_->setProgress(0.48);
    statusBadge_->set(StatusBadge::State::Busy, "Server signing");

    // Sign (server)
    api_->blindSign(poll_.pollId, token, blinded_);
}

void DashboardPage::scheduleOrSend() {
    dto::EncryptedVote v;
    v.ballotId = ballotId_;
    v.ciphertextB64 = ciphertext_;
    v.signature = finalSig_;
    v.clientTsMs = util::nowMs();

    // Put into outbox, then worker will deliver.
    OutboxItem item;
    item.id = "out_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(10);
    item.pollId = poll_.pollId;
    item.createdAtMs = util::nowMs();

    const int minS = qMax(0, settings_.delayMinSec);
    const int maxS = qMax(minS, settings_.delayMaxSec);

    qint64 submitAt = util::nowMs();
    if (settings_.delayEnabled) {
        const int delayS = QRandomGenerator::global()->bounded(minS, maxS + 1);
        submitAt += qint64(delayS) * 1000;

        connectBtn_->setState(ConnectButton::State::Scheduled);
        connectBtn_->setProgress(0.92);
        statusBadge_->set(StatusBadge::State::Busy, "Scheduled in " + util::formatEta(submitAt));
        log("Scheduled send in " + QString::number(delayS) + "s");
    } else {
        connectBtn_->setState(ConnectButton::State::Working);
        connectBtn_->setProgress(0.92);
        statusBadge_->set(StatusBadge::State::Busy, "Sending now");
    }

    item.submitAtMs = submitAt;
    item.vote = v;

    outbox_->upsert(item);
    outboxIdLast_ = item.id;

    // Fill Verify tab with last vote
    emit lastVoteForVerify(poll_.signPublicKey, messageToSign_, finalSig_);

    connectBtn_->setProgress(1.0);
    if (!settings_.delayEnabled) {
        // Worker will pick it up within 1s.
    }
}

void DashboardPage::setStatus(StatusBadge* badge, const QString& text, bool isError) {
    if (!badge) return;
    badge->set(isError ? StatusBadge::State::Error : StatusBadge::State::Ok, text);
}

void DashboardPage::log(const QString& line) {
    logEdit_->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss") + "  " + line);
}

void DashboardPage::updateOutboxInfo() {
    const int n = outbox_->items().size();
    outboxInfo_->setText("Outbox: " + QString::number(n) + " pending");
}

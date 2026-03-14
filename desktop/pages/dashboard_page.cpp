#include "dashboard_page.h"

#include <QUuid>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QRegularExpression>
#include <QScrollArea>

#include "widgets/connect_button.h"
#include "widgets/status_badge.h"
#include "../core.h"

namespace {

    QFrame *makeCard(QWidget * parent = nullptr) {
        auto *frame = new QFrame(parent);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setObjectName("dashboardCard");
        frame->setStyleSheet(
                "QFrame#dashboardCard {"
                "  background-color: #232323;"
                "  border: 1px solid #353535;"
                "  border-radius: 14px;"
                "}"
        );
        return frame;
    }

    QLabel *makeCardTitle(const QString &text, QWidget *parent = nullptr) {
        auto *label = new QLabel(text, parent);
        label->setStyleSheet("font-size: 16px; font-weight: 700;");
        return label;
    }

    QString boolToOpenClosed(bool isOpen) {
        return isOpen ? "Open" : "Closed";
    }

} // namespace

DashboardPage::DashboardPage(QWidget *parent)
        : QWidget(parent),
          api_(new ApiClient(this)) {
    api_->setBaseUrl(QUrl("http://127.0.0.1:8080/"));

    setupUi();
    setupConnections();

    advancedPanel_->setVisible(false);

    setOkState("Ready");
    appendLog("Dashboard initialized.");
    refreshForCurrentServer(true);
}

void DashboardPage::setServerUrl(const QString &url) {
    api_->setBaseUrl(QUrl(url));
    const QString normalized = api_->baseUrl().toString(QUrl::FullyDecoded);
    serverValueLabel_->setText(normalized);
    appendLog("Server URL updated: " + normalized);
    refreshForCurrentServer(true);
}

QString DashboardPage::serverUrl() const {
    return api_->baseUrl().toString(QUrl::FullyDecoded);
}

void DashboardPage::refreshForCurrentServer(bool withHealthCheck) {
    currentPoll_ = PollDetails{};
    currentPollId_.clear();
    currentPublicKey_.clear();
    currentToken_.clear();
    currentBallot_.clear();
    currentBlinded_.clear();
    currentInverse_.clear();
    currentBlindSignature_.clear();

    pollsCombo_->blockSignals(true);
    pollsCombo_->clear();
    pollsCombo_->blockSignals(false);
    candidateCombo_->clear();

    publicKeyEdit_->clear();
    tokenEdit_->clear();
    ballotEdit_->clear();
    blindedEdit_->clear();
    inverseEdit_->clear();
    blindSignatureEdit_->clear();
    finalSignatureEdit_->clear();

    updatePollInfoUi();
    setBusyState(withHealthCheck ? "Checking server..." : "Loading polls...");

    if (withHealthCheck) {
        api_->health();
    } else {
        api_->listPolls();
    }
}

void DashboardPage::setupUi() {
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *content = new QWidget(scrollArea);
    auto *root = new QVBoxLayout(content);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(14);

    // ===== top bar =====
    auto *topBar = new QHBoxLayout;
    topBar->setSpacing(10);

    auto *titleLabel = new QLabel("Blind Vote Dashboard");
    titleLabel->setStyleSheet("font-size: 22px; font-weight: 800;");

    auto *serverChip = new QLabel("Server:");
    serverChip->setStyleSheet("font-weight: 700; color: #bdbdbd;");
    serverValueLabel_ = new QLabel("http://127.0.0.1:8080/");
    serverValueLabel_->setStyleSheet(
            "padding: 6px 10px; background: #2d2d2d; border-radius: 8px;"
    );

    auto *modeChip = new QLabel("Mode:");
    modeChip->setStyleSheet("font-weight: 700; color: #bdbdbd;");
    modeValueLabel_ = new QLabel("Real API + Stub Crypto");
    modeValueLabel_->setStyleSheet(
            "padding: 6px 10px; background: #2d2d2d; border-radius: 8px;"
    );

    topBar->addWidget(titleLabel);
    topBar->addStretch();
    topBar->addWidget(serverChip);
    topBar->addWidget(serverValueLabel_);
    topBar->addSpacing(8);
    topBar->addWidget(modeChip);
    topBar->addWidget(modeValueLabel_);

    root->addLayout(topBar);

    // ===== main grid =====
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);

    auto *pollCard = makeCard(content);
    auto *pollCardLayout = new QVBoxLayout(pollCard);
    pollCardLayout->setContentsMargins(14, 14, 14, 14);
    pollCardLayout->setSpacing(10);

    pollCardLayout->addWidget(makeCardTitle("Poll", pollCard));

    pollsCombo_ = new QComboBox(pollCard);
    pollsCombo_->setMinimumHeight(34);

    auto *pollButtonsRow = new QHBoxLayout;
    refreshPollsButton_ = new QPushButton("Refresh", pollCard);
    createPollButton_ = new QPushButton("Create demo poll", pollCard);

    pollButtonsRow->addWidget(refreshPollsButton_);
    pollButtonsRow->addWidget(createPollButton_);

    pollTitleValueLabel_ = new QLabel("—", pollCard);
    pollStatusValueLabel_ = new QLabel("—", pollCard);
    pollVotesValueLabel_ = new QLabel("—", pollCard);

    auto *pollForm = new QFormLayout;
    pollForm->addRow("Title:", pollTitleValueLabel_);
    pollForm->addRow("Status:", pollStatusValueLabel_);
    pollForm->addRow("Votes:", pollVotesValueLabel_);

    pollCardLayout->addWidget(pollsCombo_);
    pollCardLayout->addLayout(pollButtonsRow);
    pollCardLayout->addLayout(pollForm);
    pollCardLayout->addStretch();

    auto *actionCard = makeCard(content);
    auto *actionCardLayout = new QVBoxLayout(actionCard);
    actionCardLayout->setContentsMargins(20, 20, 20, 20);
    actionCardLayout->setSpacing(14);

    auto *actionTitle = makeCardTitle("Vote Session", actionCard);
    actionTitle->setAlignment(Qt::AlignHCenter);

    connectButton_ = new ConnectButton(actionCard);
    connectButton_->setMinimumSize(240, 240);

    statusBadge_ = new StatusBadge(actionCard);

    actionCardLayout->addWidget(actionTitle);
    actionCardLayout->addStretch();
    actionCardLayout->addWidget(connectButton_, 0, Qt::AlignHCenter);
    actionCardLayout->addWidget(statusBadge_, 0, Qt::AlignHCenter);
    actionCardLayout->addStretch();

    auto *voteCard = makeCard(content);
    auto *voteCardLayout = new QVBoxLayout(voteCard);
    voteCardLayout->setContentsMargins(14, 14, 14, 14);
    voteCardLayout->setSpacing(10);

    voteCardLayout->addWidget(makeCardTitle("Vote", voteCard));

    candidateCombo_ = new QComboBox(voteCard);
    candidateCombo_->setMinimumHeight(34);

    delayCheckBox_ = new QCheckBox("Delay sending (stub UI only)", voteCard);
    delayCheckBox_->setChecked(true);

    minDelaySpin_ = new QSpinBox(voteCard);
    minDelaySpin_->setRange(1, 600);
    minDelaySpin_->setValue(30);
    minDelaySpin_->setSuffix(" sec");

    maxDelaySpin_ = new QSpinBox(voteCard);
    maxDelaySpin_->setRange(1, 600);
    maxDelaySpin_->setValue(90);
    maxDelaySpin_->setSuffix(" sec");

    auto *voteForm = new QFormLayout;
    voteForm->addRow("Candidate:", candidateCombo_);
    voteForm->addRow("", delayCheckBox_);
    voteForm->addRow("Min delay:", minDelaySpin_);
    voteForm->addRow("Max delay:", maxDelaySpin_);

    voteCardLayout->addLayout(voteForm);
    voteCardLayout->addStretch();

    grid->addWidget(pollCard, 0, 0);
    grid->addWidget(actionCard, 0, 1);
    grid->addWidget(voteCard, 0, 2);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 2);
    grid->setColumnStretch(2, 1);

    root->addLayout(grid);

    // ===== advanced =====
    advancedButton_ = new QPushButton("Show advanced details", content);
    root->addWidget(advancedButton_);

    advancedPanel_ = new QWidget(content);
    auto *advLayout = new QVBoxLayout(advancedPanel_);
    advLayout->setContentsMargins(0, 0, 0, 0);
    advLayout->setSpacing(12);

    auto *protocolCard = makeCard(advancedPanel_);
    auto *protocolCardLayout = new QVBoxLayout(protocolCard);
    protocolCardLayout->setContentsMargins(14, 14, 14, 14);
    protocolCardLayout->setSpacing(10);

    protocolCardLayout->addWidget(makeCardTitle("Protocol data", protocolCard));

    publicKeyEdit_ = new QLineEdit(protocolCard);
    tokenEdit_ = new QLineEdit(protocolCard);
    ballotEdit_ = new QLineEdit(protocolCard);
    blindedEdit_ = new QLineEdit(protocolCard);
    inverseEdit_ = new QLineEdit(protocolCard);
    blindSignatureEdit_ = new QLineEdit(protocolCard);
    finalSignatureEdit_ = new QLineEdit(protocolCard);

    for (QLineEdit *edit: {
            publicKeyEdit_,
            tokenEdit_,
            ballotEdit_,
            blindedEdit_,
            inverseEdit_,
            blindSignatureEdit_,
            finalSignatureEdit_
    }) {
        edit->setReadOnly(true);
    }

    auto *protocolForm = new QFormLayout;
    protocolForm->addRow("Public key:", publicKeyEdit_);
    protocolForm->addRow("Token:", tokenEdit_);
    protocolForm->addRow("Ballot:", ballotEdit_);
    protocolForm->addRow("Blinded:", blindedEdit_);
    protocolForm->addRow("Inverse:", inverseEdit_);
    protocolForm->addRow("Blind signature:", blindSignatureEdit_);
    protocolForm->addRow("Final signature:", finalSignatureEdit_);

    protocolCardLayout->addLayout(protocolForm);

    auto *logCard = makeCard(advancedPanel_);
    auto *logCardLayout = new QVBoxLayout(logCard);
    logCardLayout->setContentsMargins(14, 14, 14, 14);
    logCardLayout->setSpacing(10);

    logCardLayout->addWidget(makeCardTitle("Log", logCard));

    logEdit_ = new QPlainTextEdit(logCard);
    logEdit_->setReadOnly(true);
    logEdit_->setMinimumHeight(220);

    logCardLayout->addWidget(logEdit_);

    advLayout->addWidget(protocolCard);
    advLayout->addWidget(logCard);

    root->addWidget(advancedPanel_);
    root->addStretch(); // чтобы страница красиво тянулась

    scrollArea->setWidget(content);
    outerLayout->addWidget(scrollArea);
}

void DashboardPage::setupConnections() {
    connect(advancedButton_, &QPushButton::clicked, this, [this]() {
        const bool show = !advancedPanel_->isVisible();
        advancedPanel_->setVisible(show);
        advancedButton_->setText(show ? "Hide advanced details" : "Show advanced details");
    });

    connect(refreshPollsButton_, &QPushButton::clicked, this, [this]() {
        setBusyState("Loading polls...");
        api_->listPolls();
    });

    connect(createPollButton_, &QPushButton::clicked, this, [this]() {
        setBusyState("Creating poll...");
        {
            bool ok = false;
            QString title = QInputDialog::getText(this, "Create poll", "Poll title:", QLineEdit::Normal, "My poll",
                                                  &ok);
            if (!ok || title.trimmed().isEmpty()) return;
            QString candidatesRaw = QInputDialog::getMultiLineText(this, "Create poll",
                                                                   "Candidates (one per line or separated by commas):",
                                                                   "Alice\nBob", &ok);
            if (!ok) return;
            QStringList candidates;
            for (QString part: candidatesRaw.split(QRegularExpression("[\n,|]"), Qt::SkipEmptyParts)) {
                part = part.trimmed();
                if (!part.isEmpty()) candidates << part;
            }
            if (candidates.size() < 2) {
                setErrorState("Need at least 2 candidates");
                appendLog("Create poll failed: need at least 2 candidates.");
                return;
            }
            QString allowedRaw = QInputDialog::getMultiLineText(this, "Create poll",
                                                                "Allowed logins (one per line, comma or | separated). Leave empty = everyone.",
                                                                "", &ok);
            if (!ok) return;
            QStringList allowed;
            for (QString part: allowedRaw.split(QRegularExpression("[\n,|]"), Qt::SkipEmptyParts)) {
                part = part.trimmed();
                if (!part.isEmpty()) allowed << part;
            }
            setBusyState("Creating poll...");
            api_->createPoll(title.trimmed(), candidates, 0, currentUsername_, allowed);
        }
    });

    connect(pollsCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        const QString pollId = pollsCombo_->currentData().toString();
        if (pollId.isEmpty()) {
            return;
        }

        setBusyState("Loading poll...");
        api_->getPoll(pollId);
    });

    connect(connectButton_, &ConnectButton::clicked, this, [this]() { startVoteFlow(); });

    connect(closePollButton_, &QPushButton::clicked, this, [this]() {
        if (currentUsername_.trimmed().isEmpty()) {
            setErrorState("Username not set");
            appendLog("Open Settings and set your login first.");
            return;
        }

        if (currentPollId_.isEmpty()) {
            setErrorState("No poll selected");
            return;
        }
        if (currentUsername_.trimmed().isEmpty()) {
            setErrorState("Username not set");
            return;
        }
        setBusyState("Closing poll...");
        api_->closePoll(currentPollId_, currentUsername_);
    });

    // ===== API callbacks =====

    connect(api_, &ApiClient::healthOk, this, [this]() {
        appendLog("Server health OK.");
        setBusyState("Loading polls...");
        api_->listPolls();
    });

    connect(api_, &ApiClient::pollsListed, this, [this](const QList<PollSummary> &polls) {
        pollsCombo_->blockSignals(true);
        pollsCombo_->clear();

        for (const auto &poll: polls) {
            pollsCombo_->addItem(
                    QString("%1 (%2)").arg(poll.title, boolToOpenClosed(poll.isOpen)),
                    poll.pollId
            );
        }

        pollsCombo_->blockSignals(false);

        appendLog(QString("Loaded %1 poll(s).").arg(polls.size()));

        if (!polls.isEmpty()) {
            pollsCombo_->setCurrentIndex(0);
            api_->getPoll(polls.first().pollId);
        } else {
            currentPoll_ = PollDetails{};
            currentPollId_.clear();
            currentPublicKey_.clear();
            candidateCombo_->clear();
            updatePollInfoUi();
            setOkState("No polls available");
        }
    });

    connect(api_, &ApiClient::pollCreated, this, [this](const CreatePollResponse &r) {
        currentPollId_ = r.pollId;
        currentPublicKey_ = r.publicKey;
        currentToken_.clear();

        publicKeyEdit_->setText(currentPublicKey_);
        tokenEdit_->clear();

        appendLog("Poll created: " + r.title + " [" + r.pollId + "] owner=" + r.ownerUsername + ", allowed=" +
                  r.allowedUsernames.join(", "));
        setOkState("Poll created");

        api_->listPolls();
        api_->getPoll(r.pollId);
    });

    connect(api_, &ApiClient::pollFetched, this, [this](const PollDetails &poll) {
        currentPoll_ = poll;
        currentPollId_ = poll.pollId;
        currentPublicKey_ = poll.publicKey;

        publicKeyEdit_->setText(currentPublicKey_);

        candidateCombo_->clear();
        candidateCombo_->addItems(poll.candidates);

        updatePollInfoUi();
        appendLog("Loaded poll details: " + poll.title + ", owner=" + poll.ownerUsername);
        setOkState("Poll ready");
    });

    connect(api_, &ApiClient::tokenIssued, this, [this](const IssueTokenResponse &r) {
        if (r.tokens.isEmpty()) {
            setErrorState("Token not received");
            appendLog("Token response was empty.");
            return;
        }

        currentToken_ = r.tokens.first();
        tokenEdit_->setText(currentToken_);

        appendLog("Token issued.");
        setBusyState("Token ready, signing...");

        // continue flow automatically
        const std::string blindResult = Blind(currentPublicKey_.toStdString(),
                                              currentBallot_.toStdString());

        const auto pos = blindResult.find(':');
        if (pos == std::string::npos) {
            setErrorState("Blind failed");
            appendLog("Blind() returned invalid result.");
            return;
        }

        currentBlinded_ = QString::fromStdString(blindResult.substr(0, pos));
        currentInverse_ = QString::fromStdString(blindResult.substr(pos + 1));

        blindedEdit_->setText(currentBlinded_);
        inverseEdit_->setText(currentInverse_);

        api_->signBlinded(currentPollId_, currentToken_, currentBlinded_);
    });

    connect(api_, &ApiClient::blindedSigned, this, [this](const SignResponse &r) {
        currentBlindSignature_ = r.blindSignature;
        blindSignatureEdit_->setText(currentBlindSignature_);

        appendLog("Blind signature received from server.");

        const QString finalSignature = QString::fromStdString(
                Finalize(currentPublicKey_.toStdString(),
                         currentBallot_.toStdString(),
                         currentBlindSignature_.toStdString(),
                         currentInverse_.toStdString())
        );

        finalSignatureEdit_->setText(finalSignature);

        if (delayCheckBox_->isChecked()) {
            appendLog(
                    QString("Delay mode enabled: stub scheduling (%1..%2 sec), sending immediately for now.")
                            .arg(minDelaySpin_->value())
                            .arg(maxDelaySpin_->value())
            );
        }

        api_->submitVote(currentPollId_,
                         currentBallot_,
                         finalSignature,
                         currentToken_,
                         currentUsername_,
                         currentPassword_);
    });

    connect(api_, &ApiClient::voteSubmitted, this, [this](const SubmitVoteResponse &r) {
        appendLog("Vote submitted successfully.");
        appendLog(QString("Candidate accepted: %1, total votes: %2, user: %3")
                          .arg(r.candidate)
                          .arg(r.totalVotes)
                          .arg(r.username));

        currentToken_.clear();
        tokenEdit_->clear();

        setOkState("Vote sent");
        api_->getPoll(currentPollId_);
    });

    connect(api_, &ApiClient::resultsReceived, this, [this](const ResultsResponse &r) {
        appendLog(QString("Results loaded for poll: %1").arg(r.title));
    });

    connect(api_, &ApiClient::pollClosed, this, [this](const ClosePollResponse &r) {
        appendLog("Poll closed: " + r.pollId);
        setOkState("Poll closed");
        api_->getPoll(r.pollId);
    });

    connect(api_, &ApiClient::requestFailed, this,
            [this](const QString &where, int httpStatus, const QString &code, const QString &message) {
                appendLog(QString("[%1] HTTP %2, %3: %4")
                                  .arg(where)
                                  .arg(httpStatus)
                                  .arg(code)
                                  .arg(message));
                setErrorState("Request failed");
            });
}

void DashboardPage::appendLog(const QString &text) {
    logEdit_->appendPlainText(text);
}

void DashboardPage::setBusyState(const QString &text) {
    statusBadge_->setState(StatusBadge::State::Busy, text);
    connectButton_->setState(ConnectButton::State::Working);
    connectButton_->setProgress(0.35);
}

void DashboardPage::setOkState(const QString &text) {
    statusBadge_->setState(StatusBadge::State::Ok, text);
    connectButton_->setState(ConnectButton::State::Success);
    connectButton_->setProgress(1.0);
}

void DashboardPage::setErrorState(const QString &text) {
    statusBadge_->setState(StatusBadge::State::Error, text);
    connectButton_->setState(ConnectButton::State::Error);
    connectButton_->setProgress(1.0);
}


void DashboardPage::setUsername(const QString &username) {
    currentUsername_ = username.trimmed();
    userValueLabel_->setText(currentUsername_.isEmpty() ? QStringLiteral("—") : currentUsername_);
}

QString DashboardPage::username() const {
    return currentUsername_;
}

void DashboardPage::setPassword(const QString &password) {
    currentPassword_ = password;
}

QString DashboardPage::password() const {
    return currentPassword_;
}

void DashboardPage::updatePollInfoUi() {
    pollTitleValueLabel_->setText(currentPoll_.title.isEmpty() ? "—" : currentPoll_.title);
    pollStatusValueLabel_->setText(boolToOpenClosed(currentPoll_.isOpen));
    pollVotesValueLabel_->setText(QString::number(currentPoll_.totalVotes));
    pollOwnerValueLabel_->setText(
            currentPoll_.ownerUsername.isEmpty() ? QStringLiteral("—") : currentPoll_.ownerUsername);
    pollAllowedValueLabel_->setText(
            currentPoll_.allowedUsernames.isEmpty() ? QStringLiteral("Everyone") : currentPoll_.allowedUsernames.join(
                    ", "));
    closePollButton_->setEnabled(
            !currentPoll_.ownerUsername.isEmpty() && currentPoll_.ownerUsername == currentUsername_ &&
            currentPoll_.isOpen);
}

QString DashboardPage::generateBallotId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void DashboardPage::startVoteFlow() {
    if (currentPollId_.isEmpty()) {
        setErrorState("No poll selected");
        appendLog("Select or create a poll first.");
        return;
    }

    if (!currentPoll_.isOpen) {
        setErrorState("Poll is closed");
        appendLog("The selected poll is closed.");
        return;
    }

    const QString candidate = candidateCombo_->currentText().trimmed();
    if (candidate.isEmpty()) {
        setErrorState("No candidate selected");
        appendLog("Select a candidate first.");
        return;
    }

    if (currentPublicKey_.isEmpty()) {
        setErrorState("Public key missing");
        appendLog("Poll public key is missing.");
        return;
    }
    if (currentUsername_.trimmed().isEmpty()) {
        setErrorState("Username not set");
        appendLog("Open Settings and set your login first.");
        return;
    }

    if (currentPassword_.isEmpty()) {
        setErrorState("Password not set");
        appendLog("Open Settings and set your password first.");
        return;
    }
    const QString ballotId = generateBallotId();
    currentBallot_ = candidate + "|" + ballotId;
    ballotEdit_->setText(currentBallot_);

    appendLog("Vote session started.");
    appendLog("Ballot prepared: " + currentBallot_);

    setBusyState("Requesting token...");
    api_->issueToken(currentPollId_, currentUsername_, currentPassword_, 1);
}
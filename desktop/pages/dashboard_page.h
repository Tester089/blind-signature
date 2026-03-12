#pragma once

#include <QWidget>
#include <QString>

#include "pages/settings_page.h"
#include "net/dto.h"

class ApiClient;
class IEncryptor;
class OutboxStore;
class OutboxWorker;

class QComboBox;
class QPushButton;
class QLabel;
class QPlainTextEdit;
class QCheckBox;
class QSpinBox;
class QTextEdit;

class ConnectButton;
class StatusBadge;
class CopyField;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    DashboardPage(ApiClient* api, IEncryptor* encryptor, OutboxStore* outbox, OutboxWorker* worker, QWidget* parent = nullptr);

public slots:
    void applySettings(const UiSettings& s);

signals:
    void lastVoteForVerify(const QString& publicKey, const QString& message, const QString& signature);

private:
    void setupUi();
    void connectSignals();

    void refreshPolls();
    void onPollSelected(int idx);

    void startVote();
    void continueAfterToken();
    void scheduleOrSend();

    void setStatus(StatusBadge* badge, const QString& text, bool isError);
    void log(const QString& line);

    void updateOutboxInfo();

    ApiClient* api_;
    IEncryptor* encryptor_;
    OutboxStore* outbox_;
    OutboxWorker* worker_;

    UiSettings settings_;


    dto::PollDetails poll_;
    QStringList tokens_;


    QString candidate_;
    QString ballotId_;
    QString ballotPlain_;
    QString ciphertext_;
    QString messageToSign_;
    QString blinded_;
    QString inverse_;
    QString blindSig_;
    QString finalSig_;
    QString outboxIdLast_;


    QLabel* serverChip_;
    QComboBox* pollCombo_;
    QPushButton* refreshBtn_;
    QPushButton* createBtn_;

    QLabel* pollInfo_;
    QComboBox* candidateCombo_;

    ConnectButton* connectBtn_;
    StatusBadge* statusBadge_;
    QLabel* outboxInfo_;

    QPushButton* advancedToggle_;
    QWidget* advancedPanel_;

    CopyField* msgField_;
    CopyField* blindedField_;
    CopyField* invField_;
    CopyField* blindSigField_;
    CopyField* finalSigField_;
    CopyField* cipherField_;

    QPlainTextEdit* logEdit_;
};

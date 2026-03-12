#pragma once

#include <QWidget>
#include <QStringList>

#include "net/api_client.h"

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QSpinBox;
class ConnectButton;
class StatusBadge;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);

    void setServerUrl(const QString& url);
    QString serverUrl() const;

private:
    void setupUi();
    void setupConnections();
    void appendLog(const QString& text);
    void setBusyState(const QString& text);
    void setOkState(const QString& text);
    void setErrorState(const QString& text);

    void updatePollInfoUi();
    void startVoteFlow();
    QString generateBallotId() const;
    void refreshForCurrentServer(bool withHealthCheck = true);

private:
    ApiClient* api_;

    QString currentPollId_;
    QString currentPublicKey_;
    QString currentToken_;
    QString currentBallot_;
    QString currentBlinded_;
    QString currentInverse_;
    QString currentBlindSignature_;

    PollDetails currentPoll_;

    QLabel* serverValueLabel_;
    QLabel* modeValueLabel_;

    QComboBox* pollsCombo_;
    QPushButton* refreshPollsButton_;
    QPushButton* createPollButton_;

    QLabel* pollTitleValueLabel_;
    QLabel* pollStatusValueLabel_;
    QLabel* pollVotesValueLabel_;

    QComboBox* candidateCombo_;
    QCheckBox* delayCheckBox_;
    QSpinBox* minDelaySpin_;
    QSpinBox* maxDelaySpin_;

    ConnectButton* connectButton_;
    StatusBadge* statusBadge_;

    QPushButton* advancedButton_;
    QWidget* advancedPanel_;
    QLineEdit* publicKeyEdit_;
    QLineEdit* tokenEdit_;
    QLineEdit* ballotEdit_;
    QLineEdit* blindedEdit_;
    QLineEdit* inverseEdit_;
    QLineEdit* blindSignatureEdit_;
    QLineEdit* finalSignatureEdit_;
    QPlainTextEdit* logEdit_;
};

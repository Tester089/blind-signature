#pragma once

#include <QWidget>
#include <QStringList>

#include "net/api_client.h"

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QTextEdit;
class QPlainTextEdit;
class QCheckBox;
class QSpinBox;
class QFrame;

class ConnectButton;
class StatusBadge;

class DashboardPage : public QWidget
{
Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);

    void setServerUrl(const QString& url);

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

    // top bar
    QLabel* serverValueLabel_;
    QLabel* modeValueLabel_;

    // left card
    QComboBox* pollsCombo_;
    QPushButton* refreshPollsButton_;
    QPushButton* createPollButton_;

    QLabel* pollTitleValueLabel_;
    QLabel* pollStatusValueLabel_;
    QLabel* pollVotesValueLabel_;

    // vote card
    QComboBox* candidateCombo_;
    QCheckBox* delayCheckBox_;
    QSpinBox* minDelaySpin_;
    QSpinBox* maxDelaySpin_;

    // center area
    ConnectButton* connectButton_;
    StatusBadge* statusBadge_;

    // advanced
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
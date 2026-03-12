#pragma once

#include <QWidget>

class ConnectButton : public QWidget
{
Q_OBJECT

public:
    enum class State { Idle, Working, Success, Error };

    explicit ConnectButton(QWidget* parent = nullptr);

    void setState(State state);
    void setProgress(double value01);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    State state_;
    double progress_;
};
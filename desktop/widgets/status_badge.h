#pragma once

#include <QWidget>

class StatusBadge : public QWidget
{
Q_OBJECT
public:
    enum class State { Idle, Busy, Ok, Error };

    explicit StatusBadge(QWidget* parent = nullptr);

    void setState(State state, const QString& text);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    State state_;
    QString text_;
};
#include "connect_button.h"

#include <QPainter>
#include <QMouseEvent>

ConnectButton::ConnectButton(QWidget* parent)
        : QWidget(parent),
          state_(State::Idle),
          progress_(0.0)
{
    setCursor(Qt::PointingHandCursor);
    setMinimumSize(220, 220);
}

void ConnectButton::setState(State state)
{
    state_ = state;
    update();
}

void ConnectButton::setProgress(double value01)
{
    if (value01 < 0.0) value01 = 0.0;
    if (value01 > 1.0) value01 = 1.0;
    progress_ = value01;
    update();
}

void ConnectButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outer = rect().adjusted(12, 12, -12, -12);
    const QRectF inner = rect().adjusted(36, 36, -36, -36);

    QColor ringColor;
    QColor centerColor;

    switch (state_) {
        case State::Idle:
            ringColor = QColor(90, 90, 90);
            centerColor = QColor(40, 40, 40);
            break;
        case State::Working:
            ringColor = QColor(80, 130, 255);
            centerColor = QColor(32, 42, 72);
            break;
        case State::Success:
            ringColor = QColor(60, 180, 110);
            centerColor = QColor(28, 58, 40);
            break;
        case State::Error:
            ringColor = QColor(220, 80, 80);
            centerColor = QColor(70, 30, 30);
            break;
    }

    QPen pen(QColor(55, 55, 55), 10);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(outer);

    QPen progressPen(ringColor, 10);
    progressPen.setCapStyle(Qt::RoundCap);
    p.setPen(progressPen);
    p.drawArc(outer, 90 * 16, static_cast<int>(-360.0 * progress_ * 16.0));

    p.setPen(Qt::NoPen);
    p.setBrush(centerColor);
    p.drawEllipse(inner);

    QString text;
    switch (state_) {
        case State::Idle: text = "CAST VOTE"; break;
        case State::Working: text = "WORKING"; break;
        case State::Success: text = "SENT"; break;
        case State::Error: text = "ERROR"; break;
    }

    p.setPen(Qt::white);
    QFont f = font();
    f.setPointSize(14);
    f.setBold(true);
    p.setFont(f);
    p.drawText(inner, Qt::AlignCenter, text);
}

void ConnectButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}
#include "status_badge.h"

#include <QPainter>

StatusBadge::StatusBadge(QWidget* parent)
        : QWidget(parent),
          state_(State::Idle),
          text_("Idle")
{
    setMinimumHeight(36);
    setMinimumWidth(240);
}

void StatusBadge::setState(State state, const QString& text)
{
    state_ = state;
    text_ = text;
    update();
}

QSize StatusBadge::sizeHint() const
{
    return {260, 36};
}

void StatusBadge::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bg;
    QColor fg = Qt::white;

    switch (state_) {
        case State::Idle:  bg = QColor(70, 70, 70); break;
        case State::Busy:  bg = QColor(60, 90, 170); break;
        case State::Ok:    bg = QColor(40, 130, 80); break;
        case State::Error: bg = QColor(150, 50, 50); break;
    }

    QRect r = rect().adjusted(0, 0, -1, -1);

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(r, 10, 10);

    QFont f = font();
    f.setBold(true);
    p.setFont(f);
    p.setPen(fg);

    p.drawText(r.adjusted(14, 0, -14, 0), Qt::AlignVCenter | Qt::AlignLeft, text_);
}
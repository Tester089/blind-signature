#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;

class CardFrame : public QFrame {
    Q_OBJECT
public:
    explicit CardFrame(QString title, QWidget* parent = nullptr);

    QVBoxLayout* bodyLayout() const { return body_; }
    void setTitle(const QString& t);

private:
    QLabel* title_;
    QVBoxLayout* body_;
};

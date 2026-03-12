#pragma once

#include <QWidget>

class QLineEdit;
class QToolButton;

class CopyField : public QWidget {
    Q_OBJECT
public:
    explicit CopyField(QWidget* parent = nullptr);

    void setText(const QString& t);
    QString text() const;

    QLineEdit* lineEdit() const { return edit_; }

private:
    QLineEdit* edit_;
    QToolButton* copyBtn_;
};

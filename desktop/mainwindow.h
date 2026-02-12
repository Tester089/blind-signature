//
// Created by Egor on 12.02.2026.
//

#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    QTextEdit* messageEdit;
    QTextEdit* outputEdit;
    QPushButton* testButton;
};

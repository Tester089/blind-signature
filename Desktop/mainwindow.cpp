//
// Created by Egor on 12.02.2026.
//
#include "mainwindow.h"
#include "../core/core.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    messageEdit = new QTextEdit;
    messageEdit->setPlaceholderText("Enter message...");

    outputEdit = new QTextEdit;
    outputEdit->setReadOnly(true);

    testButton = new QPushButton("Run Blind Signature Pipeline");

    layout->addWidget(messageEdit);
    layout->addWidget(testButton);
    layout->addWidget(outputEdit);

    setCentralWidget(central);

    connect(testButton, &QPushButton::clicked, this, [=]() {

        std::string msg = messageEdit->toPlainText().toStdString();

        std::string keys = KeyGen();
        auto pos = keys.find(':');
        std::string pk = keys.substr(0, pos);
        std::string sk = keys.substr(pos + 1);

        std::string blind_res = Blind(pk, msg);
        pos = blind_res.find(':');
        std::string blinded_msg = blind_res.substr(0, pos);
        std::string inv = blind_res.substr(pos + 1);

        std::string blind_sig = BlindSign(sk, blinded_msg);
        std::string sig = Finalize(pk, msg, blind_sig, inv);
        bool ok = Verify(pk, msg, sig);

        QString result =
            "PK: " + QString::fromStdString(pk) + "\n\n" +
            "Blinded: " + QString::fromStdString(blinded_msg) + "\n\n" +
            "Signature: " + QString::fromStdString(sig) + "\n\n" +
            "Verify: " + (ok ? "OK" : "FAIL");

        outputEdit->setText(result);
    });
}

#include <QApplication>
#include <QStyleFactory>
#include <QPalette>

#include "mainwindow.h"

static void applyVpnDark(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, QColor(20, 20, 24));
    p.setColor(QPalette::WindowText, QColor(240, 240, 240));
    p.setColor(QPalette::Base, QColor(14, 14, 18));
    p.setColor(QPalette::AlternateBase, QColor(20, 20, 24));
    p.setColor(QPalette::ToolTipBase, QColor(240, 240, 240));
    p.setColor(QPalette::ToolTipText, QColor(20, 20, 24));
    p.setColor(QPalette::Text, QColor(240, 240, 240));
    p.setColor(QPalette::Button, QColor(30, 30, 36));
    p.setColor(QPalette::ButtonText, QColor(240, 240, 240));
    p.setColor(QPalette::BrightText, QColor(255, 96, 96));
    p.setColor(QPalette::Highlight, QColor(92, 146, 255));
    p.setColor(QPalette::HighlightedText, QColor(10, 10, 10));
    app.setPalette(p);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("BlindSignatureVPN");
    app.setOrganizationName("BlindSignatureTeam");

    applyVpnDark(app);

    MainWindow w;
    w.show();

    return app.exec();
}

#include "mainwindow.h"
#include <QApplication>

#ifndef APP_VERSION
#define APP_VERSION 0
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString VERSION = APP_VERSION;
    QCoreApplication::setApplicationVersion(VERSION);
    
    qDebug() << APP_VERSION;
    qDebug() << "Application Version:" << QCoreApplication::applicationVersion();

    a.setWindowIcon(QIcon(":/banner/icon.png"));
    MainWindow w;
    w.setWindowTitle("Citrahold PC v" + QCoreApplication::applicationVersion());

    w.show();

    return a.exec();
}

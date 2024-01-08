#include "mainwindow.h"
#include <QApplication>

#include <QHttpServer>
#include <QHttpServerResponse>
#include <QString>
#include <QDebug>
#include <QMessageBox>

#ifndef APP_VERSION
#define APP_VERSION 0
#endif

using namespace Qt::StringLiterals;

static inline QString host(const QHttpServerRequest &request)
{
    return QString::fromLatin1(request.value("Host"));
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString VERSION = APP_VERSION;
    QCoreApplication::setApplicationVersion(VERSION);

    qDebug() << APP_VERSION;
    qDebug() << "Application Version:" << QCoreApplication::applicationVersion();

    QHttpServer httpServer;
    httpServer.route("/", []()
                     { return "Hello world"; });

    httpServer.route("/json/", [](const QHttpServerRequest &request)
                     { 


        qDebug() << request.body();

        QJsonDocument doc = QJsonDocument::fromJson(request.body().toStdString().c_str());
        
        if (doc.isNull())
        {
            QHttpServerResponse response(QJsonObject({
                {"error", "Failed to create JSON doc."}
            }), QHttpServerResponse::StatusCode::BadRequest);
            
            return response;
        }

        QJsonObject json = doc.object();

        QJsonObject responseJSON = QJsonObject{
                           {{"example", json["example"].toString()}}
        };

        return QHttpServerResponse(responseJSON);
        
        });

    httpServer.afterRequest([](QHttpServerResponse &&resp)
                            {
        resp.setHeader("Server", "Citrahold Internal Server");
        return std::move(resp); });

    a.setWindowIcon(QIcon(":/banner/icon.png"));
    MainWindow w;
    w.setWindowTitle("Citrahold PC v" + QCoreApplication::applicationVersion());

    w.show();

    int successfulPort = -1;

    for (int port = 29170; port <= 29998; ++port)
    {
        const auto currentPort = httpServer.listen(QHostAddress::LocalHost, port);
        if (currentPort)
        {
            successfulPort = currentPort;
            break;
        }
    }

    if (successfulPort == -1)
    {
        qWarning() << QCoreApplication::translate("QHttpServerExample",
                                                  "Server failed to listen on any port.");

        QMessageBox msgBox;
        msgBox.setText("Internal server failed to listen on any port. If you're using remote transfer, don't worry!");
        msgBox.exec();
    }
    else
    {
        qInfo().noquote() << QCoreApplication::translate("QHttpServerExample", "Running on http://127.0.0.1:%1/").arg(successfulPort);
    }

    return a.exec();
}

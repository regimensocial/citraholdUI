#ifndef CITRAHOLDSERVER_H
#define CITRAHOLDSERVER_H

#include <string>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QObject>
#include <QVector>
#include <QHash>
#include <QVector>
#include <QString>
#include <filesystem>
#include <QDateTime>

using responsePair = std::pair<int, QJsonDocument>;

enum UploadType
{
    SAVES,
    EXTDATA
};

class CitraholdServer {
public:
    CitraholdServer(QString serverAddress, QString token);

    QString getTokenFromShorthandToken(QString shorthandToken);

    QString verifyTokenToSetUserID(QString fullToken);

    QJsonArray getGameIDsFromServer(UploadType type);
    void updateServerGameIDVariables();

    QVector<QString> serverGameIDSaves;
    QVector<QString> serverGameIDExtdata;

    QDateTime getLastUploadTime(UploadType type, QString gameID);

    bool checkServerIsOnline();

    QVector<QString> getSoftwareVersions();

    int upload(UploadType type, QString filePath, QString base64Data);
    int uploadMultiple(UploadType type, QJsonArray files);
    int download(UploadType type, QString gameID, std::filesystem::path gamePath);
    int downloadMultiple(UploadType type, QString gameID, std::filesystem::path gamePath);

private:
    QString serverAddress;
    QString token;
    QNetworkAccessManager *networkManager;

    void setTokenFromString(QString token);
    responsePair sendRequest(QString address, QJsonObject* dataToSend = nullptr, QString* downloadPath = nullptr); //

};

#endif // CITRAHOLDSERVER_H

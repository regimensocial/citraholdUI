#ifndef CITRAHOLDSERVER_H
#define CITRAHOLDSERVER_H

#include <string>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QObject>

using responsePair = std::pair<int, QJsonDocument>;

enum UploadType
{
    SAVES,
    EXTDATA
};

class CitraholdServer {
public:
    CitraholdServer(QString serverAddress, QString token);
    
    // This takes a raw token and flat out sets it
    

    // This will take a shorthand token and get the full token
    QString getTokenFromShorthandToken(QString shorthandToken);

    // This will take in a full token and get the user ID
    // User ID is kind of useless, but it's a good way to verify the token
    QString verifyTokenToSetUserID(QString fullToken);

    int upload(UploadType type, QString filePath, QString base64Data);

private:
    QString serverAddress;
    QString token;
    QNetworkAccessManager *networkManager;

    void setTokenFromString(QString token); //
    // internal use only
    responsePair sendRequest(QString address, QJsonObject* dataToSend = nullptr); //

};

#endif // CITRAHOLDSERVER_H

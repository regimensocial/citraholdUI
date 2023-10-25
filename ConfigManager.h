#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <filesystem>
#include <string>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>
#include <QObject>
#include "CitraholdServer.h"


class ConfigManager  {
public:
    ConfigManager();

    std::filesystem::path getSaveDirectory() const;
    std::filesystem::path getLikelyCitraDirectory(UploadType type = UploadType::SAVES) const;

    void updateConfigFile(QJsonDocument newConfig);
    void setToken(QString token);

    QJsonDocument getGameIDFile(UploadType type);
    void updateGameIDFile(UploadType type, QJsonDocument newFile);

    QJsonDocument getConfig() const;
    QString getToken() const;

    QString userID;

    bool loggedIn();


private:
    std::filesystem::path saveDirectory;
    std::filesystem::path citraDirectory;
    QJsonDocument config;

    void initialise();
};

#endif // CONFIGMANAGER_H

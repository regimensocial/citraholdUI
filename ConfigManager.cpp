#include "ConfigManager.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <QDebug>
#include "CitraholdServer.h"

ConfigManager::ConfigManager()
{
    // Constructor: Initialize the directory paths based on the platform
    initialise();
}

std::filesystem::path ConfigManager::getSaveDirectory() const
{
    return saveDirectory;
}

std::filesystem::path ConfigManager::getLikelyCitraDirectory(UploadType type) const
{
    std::filesystem::path directory = citraDirectory;

    if (type == UploadType::SAVES) {

        directory /=  "title";
    } else {
        directory /=  "extdata";
    }

    return directory;
}

void ConfigManager::setToken(QString token)
{
    QJsonDocument config = getConfig();

    QJsonObject configObject = config.object();

    configObject["token"] = token;

    config = QJsonDocument(configObject);

    updateConfigFile(config);

    //emit checkTokenInConfig("Test");
}

QJsonDocument ConfigManager::getConfig() const
{
    // config file is saveDirectory/config.json

    // If the config file doesn't exist, create it

    std::filesystem::path filePath = saveDirectory / "config.json";

    std::fstream file(filePath, std::ios::in | std::ios::out);
    if (!file)
    {
        file.open(filePath, std::ios::out);

        QJsonObject json;
        json["serverAddress"] = "http://192.168.1.152:3000";
        json["token"] = "unknown???";
        json["_note"] = "keep your token private!";

        QJsonDocument jsonDoc(json);
        QString jsonString = jsonDoc.toJson();

        file << jsonString.toStdString();
        file.close();

        return jsonDoc;
    }
    else
    {
        file.seekp(0);
        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return QJsonDocument::fromJson(fileContents.c_str());
    }


}

QJsonDocument ConfigManager::getGameIDFile(UploadType type) {
    // config file is saveDirectory/config.json

    // If the config file doesn't exist, create it

    std::filesystem::path filePath = saveDirectory / (type == UploadType::SAVES ? "gameIDSaves.json" : "gameIDExtdata.json");

    std::fstream file(filePath, std::ios::in | std::ios::out);
    if (!file)
    {
        file.open(filePath, std::ios::out);

        QJsonObject json;
        json["gameID"] = QJsonArray();
        QJsonDocument jsonDoc(json);
        QString jsonString = jsonDoc.toJson();

        file << jsonString.toStdString();
        file.close();

        return jsonDoc;
    }
    else
    {
        file.seekp(0);
        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return QJsonDocument::fromJson(fileContents.c_str());
    }
}

void ConfigManager::updateGameIDFile(UploadType type, QJsonDocument newFile) {


    std::filesystem::path filePath = saveDirectory / (type == UploadType::SAVES ? "gameIDSaves.json" : "gameIDExtdata.json");

    std::ofstream file(filePath);

    if (file.is_open())
    {

        QString jsonString = newFile.toJson();
        file << jsonString.toStdString();
        file.close();
    }
    else
    {
        qDebug() << "Failed to open the file for writing.";
    }
}

void ConfigManager::updateConfigFile(QJsonDocument newConfig)
{
    std::filesystem::path filePath = saveDirectory / "config.json";

    std::ofstream file(filePath);

    if (file.is_open())
    {
        
        QString jsonString = newConfig.toJson();
        file << jsonString.toStdString();
        file.close();
    }
    else
    {
        qDebug() << "Failed to open the file for writing.";
    }
}

void ConfigManager::initialise()
{
    // Initialise the directory paths based on the platform
#ifdef _WIN32
    // Windows
    saveDirectory = std::getenv("APPDATA");
    saveDirectory /= "Citrahold";

    citraDirectory = std::getenv("APPDATA");
    citraDirectory /= "Citra";

#elif __APPLE__
    // macOS
    saveDirectory = std::getenv("HOME");
    saveDirectory /= ".config";
    saveDirectory /= "Citrahold";

    citraDirectory = std::getenv("HOME");
    citraDirectory /= "Library";
    citraDirectory /= "Application Support";
    citraDirectory /= "Citra";


#else
    // Assume all other platforms are Linux
    saveDirectory = std::getenv("HOME");
    saveDirectory /= ".config";
    saveDirectory /= "Citrahold";

    citraDirectory = std::getenv("HOME");
    citraDirectory /= ".local";
    citraDirectory /= "share";
    citraDirectory /= "citra-emu";

#endif


    citraDirectory /= "sdmc";
    citraDirectory /= "Nintendo 3DS";
    citraDirectory /= "00000000000000000000000000000000";
    citraDirectory /= "00000000000000000000000000000000";

    if (!std::filesystem::exists(saveDirectory))
    {
        std::filesystem::create_directories(saveDirectory);
    }

    if (!std::filesystem::exists(citraDirectory))
    {
        citraDirectory = ""; // Return an empty path if the Citra directory doesn't exist
    }

    config = getConfig();
    getGameIDFile(UploadType::EXTDATA);
    getGameIDFile(UploadType::SAVES);

}

QString ConfigManager::getToken() const {
    return getConfig()["token"].toString();
}

bool ConfigManager::loggedIn() {
    return userID != "invalid";
}

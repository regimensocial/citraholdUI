#include "ConfigManager.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <QDebug>
#include "CitraholdServer.h"

ConfigManager::ConfigManager()
{
    // Constructor: Initialise the directory paths based on the platform
    initialise();
}

std::filesystem::path ConfigManager::getSaveDirectory() const
{
    return saveDirectory;
}

std::filesystem::path ConfigManager::getLikelyCitraDirectory(UploadType type)
{
    std::filesystem::path directory = citraDirectory;

    if (type == UploadType::SAVES)
    {

        directory /= "title";
    }
    else
    {
        directory /= "extdata";
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
}

QJsonDocument ConfigManager::getConfig()
{

    std::filesystem::path filePath = saveDirectory / "config.json";

    std::fstream file(filePath, std::ios::in | std::ios::out);
    if (!file)
    {
        file.open(filePath, std::ios::out);

        QJsonObject json;
        json["serverAddress"] = "https://api.citrahold.com";
        json["token"] = "invalid";
        json["_note"] = "keep your token private!";
        json["defaultCitraDirectory"] = QString(citraDirectory.string().c_str());
        json["lastUploadedGameID"] = "";
        json["lastDownloadedGameID"] = "";
        json["lastType"] = "SAVES";
        json["lastMode"] = "UPLOAD";

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

QJsonDocument ConfigManager::getGameIDFile(UploadType type)
{

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

        QJsonDocument fileAsObject = QJsonDocument::fromJson(fileContents.c_str());
        return fileAsObject;
    }
}

void ConfigManager::updateGameIDFile(UploadType type, QJsonDocument newFile)
{

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

void ConfigManager::updateConfigProperty(QString property, QJsonValue value)
{
    QJsonDocument config = getConfig();

    QJsonObject configObject = config.object();

    configObject[property] = value;

    config = QJsonDocument(configObject);

    updateConfigFile(config);
}

QString ConfigManager::getConfigProperty(QString property)
{
    QJsonDocument config = getConfig();

    QJsonObject configObject = config.object();

    return configObject[property].toString();
}

void ConfigManager::addEntryToGameIDFile(UploadType type, QString gameID, QString gameName)
{

    QJsonDocument gameIDFile = getGameIDFile(type);

    QJsonArray gameIDArray = gameIDFile["gameID"].toArray();

    qDebug() << gameID;
    if (getGamePathFromGameID(type, gameID.trimmed()) != "")
    {
        qDebug() << "Game ID already exists!";
        return;
    }

    QJsonArray newEntry;
    newEntry.append(gameID.trimmed());
    newEntry.append(gameName.trimmed());

    gameIDArray.append(newEntry);

    QJsonObject gameIDFileAsObject = gameIDFile.object();
    gameIDFileAsObject["gameID"] = gameIDArray;
    gameIDFile = QJsonDocument(gameIDFileAsObject);
    updateGameIDFile(type, gameIDFile);


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

    if (!std::filesystem::exists(saveDirectory / "oldSaves"))
    {
        std::filesystem::create_directories(saveDirectory / "oldSaves");
    }

    if (!std::filesystem::exists(saveDirectory / "oldExtdata"))
    {
        std::filesystem::create_directories(saveDirectory / "oldExtdata");
    }

    if (!std::filesystem::exists(citraDirectory))
    {
        citraDirectory = ""; // Return an empty path if the Citra directory doesn't exist
    }

    QJsonDocument config = getConfig();
    if (getConfigProperty("defaultCitraDirectory").toStdString() != citraDirectory)
    {
        citraDirectory = std::filesystem::path(getConfigProperty("defaultCitraDirectory").toStdString());
    }

    getGameIDFile(UploadType::EXTDATA);
    getGameIDFile(UploadType::SAVES);
}

std::filesystem::path ConfigManager::getGamePathFromGameID(UploadType type, QString gameID)
{

    QJsonObject gameIDFileAsObject = getGameIDFile(type).object();

    QJsonArray gameIDArray = gameIDFileAsObject["gameID"].toArray();

    for (int i = 0; i < gameIDArray.size(); ++i)
    {
        QJsonValue value = gameIDArray.at(i);
        if (gameID == value[0].toString())
        {
            return std::filesystem::path(value[1].toString().toStdString());
        }
    }

    return "";
}

QString ConfigManager::getToken()
{
    return getConfig()["token"].toString();
}

bool ConfigManager::loggedIn()
{
    return userID != "invalid";
}

bool ConfigManager::copyDirectory(const std::filesystem::path &source, const std::filesystem::path &destination)
{
    try
    {
        if (!std::filesystem::exists(source) || !std::filesystem::is_directory(source))
        {
            qDebug() << "Source directory doesn't exist or is not a directory.";
            return false;
        }

        if (!std::filesystem::exists(destination))
        {
            if (!std::filesystem::create_directories(destination))
            {
                qDebug() << "Failed to create the destination directory.";
                return false;
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(source))
        {
            const std::filesystem::path entryPath = entry.path();
            const std::filesystem::path destinationPath = destination / entryPath.filename();

            if (std::filesystem::is_directory(entryPath))
            {
                if (!copyDirectory(entryPath, destinationPath))
                {
                    return false;
                }
            }
            else if (std::filesystem::is_regular_file(entryPath))
            {
                std::filesystem::copy_file(entryPath, destinationPath, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::filesystem::path ConfigManager::getOldSaveDirectory(UploadType type) const
{
    return saveDirectory / ((type == UploadType::SAVES) ? "oldSaves" : "oldExtdata");
}

void ConfigManager::removeEntryFromGameIDFile(UploadType type, QString gameID) {
    QJsonDocument gameIDFile = getGameIDFile(type);

    QJsonArray gameIDArray = gameIDFile["gameID"].toArray();

    for (int i = 0; i < gameIDArray.size(); ++i)
    {
        QJsonValue value = gameIDArray.at(i);
        if (gameID == value[0].toString())
        {
            gameIDArray.removeAt(i);
            break;
        }
    }

    QJsonObject gameIDFileAsObject = gameIDFile.object();
    gameIDFileAsObject["gameID"] = gameIDArray;
    gameIDFile = QJsonDocument(gameIDFileAsObject);
    updateGameIDFile(type, gameIDFile);
}
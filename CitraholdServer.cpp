/*

This class is CitraholdServer, a way to communicate with a server for if the software is being used as a client.
This might need a refactor to change some names soon

*/

#include "CitraholdServer.h"
#include "QtCore/qjsonarray.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QObject>
#include <QFile>
#include <QDir>

#include <filesystem>
#include <iostream>

CitraholdServer::CitraholdServer(QString serverAddress, QString token)
{
    this->serverAddress = serverAddress;
    this->token = token;
    this->networkManager = new QNetworkAccessManager();
}

void CitraholdServer::setTokenFromString(QString token)
{

    this->token = token;
}

QVector<QString> CitraholdServer::getSoftwareVersions()
{
    QVector<QString> versions;

    responsePair response = sendRequest(this->serverAddress + "/softwareVersion");

    if (response.first == 200)
    {
        QJsonArray versionsArray = response.second["pc"].toArray();

        for (int i = 0; i < versionsArray.size(); ++i)
        {
            versions.append(versionsArray.at(i).toString());
        }
    }

    return versions;
}

responsePair CitraholdServer::sendRequest(QString address, QJsonObject *dataToSend, QString *downloadPath)
{
    QEventLoop eventLoop;

    QObject::connect(networkManager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QNetworkRequest req(address);

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObject = QJsonObject();
    if (dataToSend != nullptr)
    {
        jsonObject = *dataToSend;
    }

    QByteArray jsonData = QJsonDocument(jsonObject).toJson();

    QNetworkReply *reply = this->networkManager->post(req, jsonData);

    eventLoop.exec();

    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError)
    {

        QVariant contentType = reply->header(QNetworkRequest::ContentTypeHeader);

        if (contentType.toString().startsWith("application/json"))
        {

            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            delete reply;
            return std::make_pair(httpStatusCode, response);
        }
        else if (downloadPath != nullptr)
        {
            QFile file(*downloadPath);

            if (file.open(QIODevice::WriteOnly))
            {
                file.write(reply->readAll());
                file.close();
                qDebug() << "Downloaded to" << *downloadPath;
            }
            else
            {
                qDebug() << "Failed to open file for writing.";
            }

            delete reply;
            return std::make_pair(httpStatusCode, QJsonDocument());
        }
    }
    else // actual error
    {

        QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        delete reply;
        return std::make_pair(httpStatusCode, response);
    }

    delete reply;
    return std::make_pair(0, QJsonDocument());
}

QString CitraholdServer::getTokenFromShorthandToken(QString shorthandToken)
{
    QJsonObject data;

    data["shorthandToken"] = shorthandToken;

    // this will return a full token
    responsePair response = sendRequest(this->serverAddress + "/getToken", &data);
    if (response.first == 200)
    {

        this->token = response.second["token"].toString();
        return response.second["token"].toString();
    }
    else
    {
        // we didn't get a token
        // TODO: handle this????
        return "invalid";
    }
}

QString CitraholdServer::verifyTokenToSetUserID(QString fullToken)
{
    QJsonObject data;

    data["token"] = fullToken;

    responsePair response = sendRequest(this->serverAddress + "/getUserID", &data);
    if (response.first == 200)
    {
        this->token = fullToken;
        return response.second["userID"].toString();
    }
    else
    {
        // we didn't get a token
        // TODO: handle this
        return "invalid";
    }
}

bool CitraholdServer::checkServerIsOnline()
{
    responsePair response = sendRequest(this->serverAddress + "/areyouawake");
    return response.first == 200;
}

int CitraholdServer::upload(UploadType type, QString filePath, QString base64Data)
{

    QJsonObject data;

    data["token"] = this->token;
    data["filename"] = filePath;
    data["data"] = base64Data;

    // this will return a full token
    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/uploadSaves" : "/uploadExtdata"), &data);

    qDebug() << "uploadResponse " << response.first;

    return response.first;
}

int CitraholdServer::uploadMultiple(UploadType type, QJsonArray files)
{

    QJsonObject data;

    data["token"] = this->token;
    data["multi"] = files;

    // this will return a full token
    qDebug() << "uploading multiple files";
    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/uploadMultiSaves" : "/uploadMultiExtdata"), &data);

    qDebug() << "uploadResponse " << response.first;

    return response.first;
}

QJsonArray CitraholdServer::getGameIDsFromServer(UploadType type)
{

    QJsonObject data;

    data["token"] = this->token;
    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/getSaves" : "/getExtdata"), &data);

    if (response.first == 200)
    {
        return response.second["games"].toArray();
    }
    else
    {
        return {};
    }
}

int CitraholdServer::download(UploadType type, QString gameID, std::filesystem::path gamePath)
{
    QJsonObject data;

    data["token"] = this->token;
    data["game"] = gameID;

    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/downloadSaves" : "/downloadExtdata"), &data);

    if (response.first == 200)
    {
        QJsonArray files = response.second["files"].toArray();

        bool successfulSoFar = true;

        for (int i = 0; i < files.size(); ++i)
        {

            if (successfulSoFar)
            {
                data["file"] = files.at(i).toString();

                std::filesystem::path value = files.at(i).toString().toStdString();

                QString downloadPath = QString::fromStdString((gamePath / value).string());

                if (downloadPath.contains("citraholdDirectoryDummy"))
                {
                    if (!std::filesystem::exists(downloadPath.toStdString()) || !std::filesystem::is_directory(downloadPath.toStdString()))
                    {
                        std::filesystem::path dir = downloadPath.toStdString();

                        if (std::filesystem::create_directories(dir.parent_path()))
                        {
                            qDebug() << "Directory created successfully.";
                        }
                    }
                }
                else
                {
                    responsePair downloadResponse = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/downloadSaves" : "/downloadExtdata"), &data, &downloadPath);

                    // qDebug() << "downloadResponse " << downloadResponse.first;
                    successfulSoFar = (downloadResponse.first == 200);

                    // qDebug() << "downloadPath " << downloadPath;
                    if (successfulSoFar && (downloadPath.contains("citraholdDirectoryDummy")))
                    {
                        // QFile::remove(downloadPath);
                        qDebug() << "Dummy file removed";
                    }
                }
            }
        }

        return successfulSoFar;
    }
    return 0;
}

int CitraholdServer::downloadMultiple(UploadType type, QString gameID, std::filesystem::path gamePath)
{

    QJsonObject data;

    data["token"] = this->token;
    data["game"] = gameID;

    responsePair response = sendRequest(this->serverAddress + (type == UploadType::EXTDATA ? "/downloadMultiExtdata" : "/downloadMultiSaves"), &data);

    qDebug() << response.first;
    if (response.first == 200)
    {
        bool successfulSoFar = true;

        QJsonDocument responseJSON = (response.second);
        QJsonArray files = responseJSON["files"].toArray();
        int numberOfItems = files.size();
        int itemNumber = 0;

        qDebug() << "Retrieved " << numberOfItems << " files\n";

        for (const QJsonValueRef &element : files)
        {
            if (!successfulSoFar)
            {
                return false;
            }

            itemNumber++;

            std::string filename = element[0].toString().toStdString();
            QString base64Data = element[1].toString();

            std::filesystem::path downloadPath = (gamePath / filename).string();

            QByteArray base64DataDecoded = QByteArray::fromBase64(base64Data.toUtf8());

            // write the file
            std::filesystem::path parentPath = downloadPath.parent_path();

            if (!std::filesystem::exists(parentPath))
            {
                std::filesystem::create_directories(parentPath);
            }

            if (filename.find("citraholdDirectoryDummy") == std::string::npos)
            {

                QFile file(downloadPath);

                if (file.open(QIODevice::WriteOnly))
                {
                    file.write(base64DataDecoded);

                    file.close();
                }
                else
                {
                    qDebug() << "Error opening the file for writing.";
                }
            }
            else
            {
                qDebug() << "[" << itemNumber << "/" << numberOfItems << "] "
                         << "Ignoring dummy file " << filename << "\n";
            }
        }

        return response.first;
    }
    return false;
}

void CitraholdServer::updateServerGameIDVariables()
{

    QVector<QString> *serverGameID;
    UploadType type;

    for (int i = 0; i <= 1; ++i)
    {
        if (i == 0)
        {
            serverGameID = &serverGameIDSaves;
            type = UploadType::SAVES;
        }
        else
        {
            serverGameID = &serverGameIDExtdata;
            type = UploadType::EXTDATA;
        }
        serverGameID->clear();
        QJsonArray gameIDs = CitraholdServer::getGameIDsFromServer(type);
        for (int i = 0; i < gameIDs.size(); ++i)
        {
            QJsonValue gameValue = gameIDs.at(i);
            if (gameValue.isString())
            {
                serverGameID->append(gameValue.toString());
            }
        }
    }
}

QDateTime CitraholdServer::getLastUploadTime(UploadType type, QString gameID)
{
    QJsonObject data;

    data["token"] = this->token;
    data["game"] = gameID;

    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/getSavesLastUpdated" : "/getExtdataLastUpdated"), &data);

    if (response.first == 200)
    {
        return QDateTime::fromString(response.second["lastModified"].toString(), Qt::ISODate);
    }
    else
    {
        return QDateTime();
    }
}
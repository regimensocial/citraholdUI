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
responsePair CitraholdServer::sendRequest(QString address, QJsonObject *dataToSend, QString* downloadPath)
{
	QEventLoop eventLoop;

    QObject::connect(networkManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

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

        if (contentType.toString().startsWith("application/json")) {
            
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            delete reply;
            return std::make_pair(httpStatusCode, response);
        } else if (downloadPath != nullptr) {
            QFile file(*downloadPath);

            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                qDebug() << "Downloaded to" << *downloadPath;
            } else {
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
    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/UploadSaves" : "/UploadExtdata"), &data);

    qDebug() << "uploadResponse " << response.first;

    return response.first;
}

QJsonArray CitraholdServer::getGameIDsFromServer(UploadType type) {

    QJsonObject data;

    data["token"] = this->token;
    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/getSaves" : "/getExtdata"), &data);


    if (response.first == 200) {
        return response.second["games"].toArray();
    } else {
        return {};
    }

}

int CitraholdServer::download(UploadType type, QString gameID, std::filesystem::path gamePath) {
    QJsonObject data;

    data["token"] = this->token;
    data["game"] = gameID;

    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/downloadSaves" : "/downloadExtdata"), &data);

    if (response.first == 200) {
        QJsonArray files = response.second["files"].toArray();

        bool successfulSoFar = true;

        for (int i = 0; i < files.size(); ++i) {

            if (successfulSoFar) {
                data["file"] = files.at(i).toString();

                std::filesystem::path value = files.at(i).toString().toStdString();

                QString downloadPath = QString::fromStdString((gamePath / value).string());

                responsePair downloadResponse = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/downloadSaves" : "/downloadExtdata"), &data, &downloadPath);

                successfulSoFar = (downloadResponse.first == 200);
            }
        }

        return successfulSoFar;

    }
    return 0;
}

void CitraholdServer::updateServerGameIDVariables() {

    QVector<QString>* serverGameID;
    UploadType type;

    for (int i = 0; i <= 1; ++i) {
        if (i == 0) {
            serverGameID = &serverGameIDSaves;
            type = UploadType::SAVES;
        } else {
            serverGameID = &serverGameIDExtdata;
            type = UploadType::EXTDATA;
        }
        serverGameID->clear();
        QJsonArray gameIDs = CitraholdServer::getGameIDsFromServer(type);
        for (int i = 0; i < gameIDs.size(); ++i) {
            QJsonValue gameValue = gameIDs.at(i);
            if (gameValue.isString()) {
                serverGameID->append(gameValue.toString());
            }
        }
    }
}

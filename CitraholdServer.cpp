#include "CitraholdServer.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QObject>

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
responsePair CitraholdServer::sendRequest(QString address, QJsonObject *dataToSend)
{
	QEventLoop eventLoop;

	// Create the HTTP request
    QObject::connect(networkManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

	QNetworkRequest req(address);

	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// Create a JSON object or use a QJsonObject

	QJsonObject jsonObject = QJsonObject();
	if (dataToSend != nullptr)
	{
		jsonObject = *dataToSend;
	}

	// Serialize the JSON object to a QByteArray
	QByteArray jsonData = QJsonDocument(jsonObject).toJson();

	// Initiate the network request asynchronously
	QNetworkReply *reply = this->networkManager->post(req, jsonData);

	// Connect the finished() signal to a slot to handle the response
	eventLoop.exec();

	if (reply->error() == QNetworkReply::NoError)
	{
		// The request was successfully made without network issues.

		// Check the HTTP status code to determine the server's response.

		int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
		delete reply;

		return std::make_pair(httpStatusCode, response);
	}
	else // actual connection error???
	{
        qDebug() << "Error!?!?";
		delete reply;
		return std::make_pair(0, QJsonDocument());
	}

	// just in case
	return std::make_pair(0, QJsonDocument());
}

QString CitraholdServer::getTokenFromShorthandToken(QString shorthandToken)
{
	// we need to make {token: token}
	QJsonObject data;

	data["shorthandToken"] = shorthandToken;

	// this will return a full token
	responsePair response = sendRequest(this->serverAddress + "/getToken", &data);
	if (response.first == 200)
	{
		// we got a token
		// debug output
		qDebug() << response.second["token"].toString();

        this->token = response.second["token"].toString();
        return response.second["token"].toString();
	}
	else
	{
		// we didn't get a token
		// TODO: handle this
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
		// we got a token
		// debug output
		qDebug() << response.second["token"].toString();

		return response.second["userID"].toString();
	}
	else
	{
		// we didn't get a token
		// TODO: handle this
		return "invalid";
	}
}

int CitraholdServer::upload(UploadType type, QString filePath, QString base64Data) {

    qDebug() << this->serverAddress << this->token;


    QJsonObject data;

    data["data"] = base64Data;
    data["filename"] = filePath;
    data["token"] = this->token;

    // this will return a full token
    responsePair response = sendRequest(this->serverAddress + (type == UploadType::SAVES ? "/uploadSaves" : "/UploadExtdata"), &data);
    return response.first;
}

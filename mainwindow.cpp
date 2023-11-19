#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QFileDialog>
#include <algorithm>
#include <sstream>
#include <QDir>
#include <QQueue>
#include <QByteArray>
#include <QTextStream>
#include <QFile>
#include <chrono>
#include <iomanip>
#include <QVBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QMap>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    configManager = new ConfigManager();

    citraholdServer = new CitraholdServer(
        configManager->getConfigProperty("serverAddress"),
        configManager->getConfigProperty("token"));

    ui->setupUi(this);
    ui->statusbar->showMessage("Not logged in.");

    ui->selectionBox->hide();

    configManager = new ConfigManager();

    connect(
        ui->verifyButton, &QPushButton::clicked,
        this, &MainWindow::handleVerifyButtonClicked);

    connect(
        ui->tokenText, &QPlainTextEdit::textChanged,
        this, &MainWindow::manageTokenTextEdit);

    connect(ui->selectDirectoryButton, &QPushButton::clicked, this, [=]()
            { handleDirectoryButton(true); });

    connect(ui->uploadButton, &QPushButton::clicked, this, &MainWindow::handleUploadButton);

    connect(ui->directoryText, &QPlainTextEdit::textChanged, this, [=]()
            { handleDirectoryButton(false); });

    connect(ui->selectPreviousGameIDsButton, &QPushButton::clicked, this, &MainWindow::openGameIDSelector);
    connect(ui->saveGameIDButton, &QPushButton::clicked, this, &MainWindow::addGameIDToFile);
    connect(ui->gameIDText, &QPlainTextEdit::textChanged, this, [&]()
            { ui->saveGameIDButton->setDisabled(false); });

    connect(ui->extdataRadio, &QRadioButton::clicked, this, &MainWindow::handleSaveExtdataRadios);
    connect(ui->saveRadio, &QRadioButton::clicked, this, &MainWindow::handleSaveExtdataRadios);
    connect(ui->fetchButton, &QPushButton::clicked, this, &MainWindow::handleServerFetch);

    connect(ui->downloadButton, &QPushButton::clicked, this, &MainWindow::handleDownloadButton);

    connect(ui->clearOldSavesButton, &QPushButton::clicked, this, &MainWindow::handleClearOldSavesButton);

    connect(ui->switchModeButton, &QPushButton::clicked, this, &MainWindow::handleToggleModeButton);

    QString userID = citraholdServer->verifyTokenToSetUserID(configManager->getToken());
    configManager->userID = userID;

    ui->directoryText->setPlainText(QString::fromStdString(configManager->getLikelyCitraDirectory().u8string()));

    if (configManager->loggedIn())
    {

        handleSuccessfulLogin();
    }
    else
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->selectionBox->hide();
        // at some point, we'll need to check if the server is running
    }

    changedNameOrDirectorySinceSetAutomatically = false;

    if (!citraholdServer->checkServerIsOnline())
    {
        showErrorBox("The server is inaccessible at this time. Please try again later.");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openGameIDSelector()
{
    QDialog dialog;
    dialog.setWindowTitle("You must now select a game ID");

    QVBoxLayout layout(&dialog);

    QJsonObject jsonObject = configManager->getGameIDFile(MainWindow::savesOrExtdata()).object();

    QJsonArray gameIDArray = jsonObject["gameID"].toArray();

    QComboBox comboBox;

    QString ofWhich = (MainWindow::savesOrExtdata() == UploadType::SAVES ? "Saves" : "Extdata");
    comboBox.addItem("Select one please (" + ofWhich + ")");

    QVector<QString> gameIDPaths;

    for (const QJsonValue &arrayValue : gameIDArray)
    {
        if (arrayValue.isArray())
        {
            QJsonArray innerArray = arrayValue.toArray();
            comboBox.addItem(innerArray[0].toString());
            gameIDPaths.append(innerArray[1].toString());
        }
    }

    // Add the combo box to the layout
    layout.addWidget(&comboBox);

    QHBoxLayout editLayout;
    QPlainTextEdit gameID;
    QPlainTextEdit gamePath;
    gameID.setLineWrapMode(QPlainTextEdit::NoWrap);
    gameID.setMaximumHeight(30);
    gameID.setMinimumHeight(30);
    gamePath.setLineWrapMode(QPlainTextEdit::NoWrap);
    gamePath.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameID.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gamePath.setMaximumHeight(30);
    gamePath.setMinimumHeight(30);
    gamePath.setMaximumWidth(300);
    gameID.setMaximumWidth(300);
    editLayout.addWidget(&gameID);
    editLayout.addWidget(&gamePath);

    layout.addLayout(&editLayout);

    QLabel label;
    layout.addWidget(&label);
    label.setMaximumWidth(600);
    label.setWordWrap(true);
    label.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    QPushButton okButton("OK");
    layout.addWidget(&okButton);

    gameID.setDisabled(true);
    gamePath.setDisabled(true);
    QObject::connect(&comboBox, &QComboBox::currentTextChanged, &dialog, [&]()
                     {
        if (comboBox.currentIndex() == 0) {
            label.setText("");
            gameID.setDisabled(true);
            gamePath.setDisabled(true);
        } else {
            gameID.setDisabled(false);
            gamePath.setDisabled(false);
            gameID.setPlainText(comboBox.currentText());
            gamePath.setPlainText(gameIDPaths[comboBox.currentIndex() - 1]);
            label.setText(comboBox.currentText() + " refers to \n" + gameIDPaths[comboBox.currentIndex() - 1]);
        } });

    QObject::connect(&okButton, &QPushButton::clicked, &dialog, [&]()
                     {
        if (comboBox.currentIndex() > 0) {
            ui->gameIDText->setPlainText(comboBox.currentText());
            ui->directoryText->setPlainText(gameIDPaths[comboBox.currentIndex() - 1]);
            ui->saveGameIDButton->setDisabled(true);
            changedNameOrDirectorySinceSetAutomatically = false;
        } else {
            ui->saveGameIDButton->setDisabled(false);
        }
        QJsonObject newGameIDObject;
        QJsonArray newGameIDArray;

        bool anyInvalidGameIDs = false;

        for (int i = 1; i < comboBox.count(); i++) {
            if ((comboBox.itemText(i) != "") && (gameIDPaths[i - 1] != "")) {
                QJsonArray entry;
                entry.append(comboBox.itemText(i).trimmed());
                entry.append(gameIDPaths[i - 1].trimmed());

                if (!std::filesystem::exists(gameIDPaths[i - 1].trimmed().toStdString())) {
                    anyInvalidGameIDs = true;
                }

                newGameIDArray.append(entry);
            }
        }

        if (anyInvalidGameIDs) {
            showErrorBox("Some of your game IDs are invalid. Please fix them.");
            return;
        }

        newGameIDObject["gameID"] = newGameIDArray;
        configManager->updateGameIDFile(MainWindow::savesOrExtdata(), QJsonDocument(newGameIDObject));

        dialog.close(); });

    QObject::connect(&gameID, &QPlainTextEdit::textChanged, &dialog, [&]()
                     {
        if (gameID.toPlainText() != comboBox.currentText()) {
            comboBox.setItemText(comboBox.currentIndex(), gameID.toPlainText());
            gameID.moveCursor(QTextCursor::End);
        } });

    QObject::connect(&gamePath, &QPlainTextEdit::textChanged, &dialog, [&]()
                     {
        if (gamePath.toPlainText() != gameIDPaths[comboBox.currentIndex() - 1]) {
            gameIDPaths[comboBox.currentIndex() - 1] = gamePath.toPlainText();
            gamePath.moveCursor(QTextCursor::End);
            label.setText(comboBox.currentText() + " points to " + gameIDPaths[comboBox.currentIndex() - 1]);
        } });

    // VALIDATE ANY PATHS!!!!!

    dialog.exec();
}

void MainWindow::handleToggleModeButton()
{
    if (ui->stackedWidget->currentIndex() == 1)
    {
        ui->stackedWidget->setCurrentIndex(2);
        ui->switchModeButton->setText("Switch to Upload");
        configManager->updateConfigProperty("lastMode", "UPLOAD");
    }
    else
    {
        ui->stackedWidget->setCurrentIndex(1);
        ui->switchModeButton->setText("Switch to Download");
        configManager->updateConfigProperty("lastMode", "DOWNLOAD");
    }
}

void MainWindow::manageTokenTextEdit()
{
    if (ui->tokenText->toPlainText().length() > 36)
    {
        ui->tokenText->setPlainText(ui->tokenText->toPlainText().left(36));
    }
}

void MainWindow::handleVerifyButtonClicked()
{
    QJsonObject data;

    ui->tokenText->setPlainText(ui->tokenText->toPlainText().trimmed());

    if (ui->tokenText->toPlainText().length() <= 6)
    { // Handle a shorthand token

        QString fullToken = citraholdServer->getTokenFromShorthandToken(ui->tokenText->toPlainText());
        if (fullToken != "invalid")
        {

            ui->statusbar->showMessage("Logged in.");
            QString userID = citraholdServer->verifyTokenToSetUserID(fullToken);
            if (userID != "invalid")
            {
                configManager->userID = userID;
                configManager->setToken(fullToken);
                ui->tokenText->setPlainText("");
                ui->tokenOutput->setText("Successfully authenticated.");

                ui->stackedWidget->setCurrentIndex(1);
                ui->selectionBox->show();
            }
            else
            {
                if (citraholdServer->checkServerIsOnline())
                {
                    ui->tokenOutput->setText("You have the wrong credentials.");
                }
                else
                {
                    ui->tokenOutput->setText("The server is inaccessible.");
                }
            }
        }
        else
        {
            if (citraholdServer->checkServerIsOnline())
            {
                ui->tokenOutput->setText("You have the wrong credentials.");
            }
            else
            {
                ui->tokenOutput->setText("The server is inaccessible.");
            }
        }
    }
    else // full token
    {
        QString fullToken = ui->tokenText->toPlainText();
        QString userID = citraholdServer->verifyTokenToSetUserID(fullToken);
        if (userID != "invalid")
        {

            configManager->setToken(fullToken);
            configManager->userID = userID;

            ui->tokenText->setPlainText("");

            ui->tokenOutput->setText("Successfully authenticated.");

            handleSuccessfulLogin();
        }
        else
        {
            if (citraholdServer->checkServerIsOnline())
            {
                ui->tokenOutput->setText("You have the wrong credentials.");
            }
            else
            {
                ui->tokenOutput->setText("The server is inaccessible.");
            }
        }
    }
}

void MainWindow::handleSuccessfulLogin()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->selectionBox->show();
    ui->statusbar->showMessage("Logged in.");
    handleServerFetch();

    QJsonObject config = configManager->getConfig().object();
    if (config["lastUploadedGameID"].toString() != "")
    {
        ui->gameIDText->setPlainText(config["lastUploadedGameID"].toString());

        if (configManager->getConfigProperty("lastMode") != "UPLOAD")
        {

            if (config["lastUploadedType"].toString() == "EXTDATA")
            {
                ui->extdataRadio->toggle();
            }
        }
        else
        {
            if (config["lastDownloadedType"].toString() == "EXTDATA")
            {
                ui->extdataRadio->toggle();
            }
        }

        configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), config["lastUploadedGameID"].toString());
    }

    if (configManager->getConfigProperty("lastMode") == "UPLOAD")
    {
        handleToggleModeButton();
    }
}

void MainWindow::handleDirectoryButton(bool openSelection)
{

    changedNameOrDirectorySinceSetAutomatically = true;
    QString directory;
    ui->saveGameIDButton->setDisabled(false);

    if (openSelection)
    {
        directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), ui->directoryText->toPlainText());
    }
    else
    {
        directory = ui->directoryText->toPlainText();
    }
    if (directory != "")
    {
        if (openSelection)
        {
            ui->directoryText->setPlainText(directory);
        }

        if (directory.contains("extdata"))
        {
            ui->extdataRadio->toggle();
        }
        else
        {
            ui->saveRadio->toggle();
        }
    }
}

void MainWindow::showErrorBox(QString error)
{
    QMessageBox errorMessage;
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.setWindowTitle("Error");
    errorMessage.setText("An error has occurred.\n");
    if (error != "")
    {
        errorMessage.setText(errorMessage.text() + "\n" + error);
    }

    errorMessage.setStandardButtons(QMessageBox::Ok);

    // Show the error message dialog
    errorMessage.exec();
}

void MainWindow::showSuccessBox(QString text)
{
    QMessageBox errorMessage;
    errorMessage.setWindowTitle("Success");
    errorMessage.setText(text);
    errorMessage.setStandardButtons(QMessageBox::Ok);

    // Show the error message dialog
    errorMessage.exec();
}

UploadType MainWindow::savesOrExtdata()
{
    return (ui->extdataRadio->isChecked() ? UploadType::EXTDATA : UploadType::SAVES);
}

void MainWindow::handleUploadButton()
{

    QString directory = ui->directoryText->toPlainText();

    // todo: check directory is valid

    QString gameID = ui->gameIDText->toPlainText();

    if (gameID.trimmed() == "")
    {
        showErrorBox("Invalid game ID");
        return;
    }

    QQueue<QDir> directoriesToVisit;
    directoriesToVisit.enqueue(directory);

    QQueue<QDir> allFilesFull;
    QQueue<QDir> allFilesRelative;

    QDateTime currentDateTime = QDateTime::currentDateTime();

    QString formattedDateTime = currentDateTime.toString("yyyyMMdd-hhmmss");

    QVector<int> responses;

    while (!directoriesToVisit.isEmpty())
    {
        QDir currentDir = directoriesToVisit.dequeue();

        citraholdServer->upload(MainWindow::savesOrExtdata(), gameID + "/" + QDir(directory).relativeFilePath(currentDir.path()) + "/" + "citraholdDirectoryDummy", "citraholdDirectoryDummy");

        QStringList entryList = currentDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString &entry : entryList)
        {
            const QString fullPath = currentDir.filePath(entry);
            if (QFileInfo(fullPath).isDir())
            {
                directoriesToVisit.enqueue(QDir(fullPath));
            }
            else
            {
                allFilesFull.enqueue(fullPath);

                QString relativePath = QDir(directory).relativeFilePath(fullPath);
                allFilesRelative.enqueue(relativePath);
            }
        }
    }


    for (const QDir &filePath : allFilesFull)
    {
        QFile file(filePath.path());

        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray fileData = file.readAll();
            file.close();

            QString base64Data = QString(fileData.toBase64());

            // CHPC means Citrahold PC, the one from the 3DS will have CHDS instead
            responses.append(citraholdServer->upload(MainWindow::savesOrExtdata(), gameID + "/" + allFilesRelative.first().path(), base64Data));
        }
        else
        {
            // Handle the case where the file couldn't be opened????
        }

        allFilesRelative.removeFirst();
    }

    int responseCode = 0;
    for (int value : responses)
    {
        responseCode = value;
        if (value != 201)
        {
            break; // Exit the loop as soon as a non-201 value is encountered
        }
    }

    if (responseCode == 201)
    {
        MainWindow::showSuccessBox("Upload successful!");

        configManager->updateConfigProperty("lastUploadedGameID", gameID);
        configManager->updateConfigProperty("lastUploadedType", (ui->extdataRadio->isChecked() ? "EXTDATA" : "SAVES"));

        handleServerFetch();
    }
    else
    {
        MainWindow::showErrorBox("Something went wrong during upload, HTTP " + QString::number(responseCode));
    }
}

void MainWindow::addGameIDToFile()
{

    QJsonDocument gameIDFile = configManager->getGameIDFile(MainWindow::savesOrExtdata());

    QJsonValue gameIDValue = gameIDFile["gameID"];

    if (gameIDValue.isArray())
    {
        QJsonArray newGameIDArray = gameIDValue.toArray();

        QString directory = ui->directoryText->toPlainText();

        if (!std::filesystem::exists(directory.toStdString()))
        {
            showErrorBox("Invalid directory");
            return;
        }

        QString gameID = ui->gameIDText->toPlainText();

        if ((gameID != "") && (directory != ""))
        {
            configManager->addEntryToGameIDFile(MainWindow::savesOrExtdata(), gameID, directory);
            ui->saveGameIDButton->setDisabled(true);
        }
    }
}

void MainWindow::handleServerFetch()
{
    ui->downloadGameIDComboBox->clear();
    citraholdServer->updateServerGameIDVariables();

    QVector<QString> *serverGameIDs = (MainWindow::savesOrExtdata() == UploadType::SAVES) ? &citraholdServer->serverGameIDSaves : &citraholdServer->serverGameIDExtdata;

    ui->downloadButton->setEnabled((serverGameIDs->size() > 0));

    for (const QString &key : *serverGameIDs)
    {

        ui->downloadGameIDComboBox->addItem(key);
    }
}

void MainWindow::handleDownloadButton()
{

    QJsonArray gameIDsOnFile = configManager->getGameIDFile(MainWindow::savesOrExtdata())["gameID"].toArray();

    if (gameIDsOnFile.size() == 0)
    {
        showErrorBox("You need to add " + ui->downloadGameIDComboBox->currentText().trimmed() + " to your gameID list.");

        QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), ui->directoryText->toPlainText());

        if (directory != "")
        {
            configManager->addEntryToGameIDFile(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText(), directory);

            if (configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()) != "")
            {
                return handleDownloadButton();
            }
        }
    }

    bool gameIDpresent = false;

    for (int i = 0; i < gameIDsOnFile.size(); ++i)
    {
        QJsonValue value = gameIDsOnFile.at(i);
        if (ui->downloadGameIDComboBox->currentText() == value[0].toString())
        {
            gameIDpresent = true;
        }
    }

    if (!gameIDpresent)
    {
        showErrorBox("You need to add " + ui->downloadGameIDComboBox->currentText().trimmed() + " to your gameID list.");

        QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), ui->directoryText->toPlainText());

        // add directory to appropriate gameID file
        if (directory != "")
        {
            configManager->addEntryToGameIDFile(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText(), directory);

            if (configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()) != "")
            {
                return handleDownloadButton();
            }
        }
    }
    else
    {

        QDateTime currentDateTime = QDateTime::currentDateTime();
        QString formattedDateTime = currentDateTime.toString("yyyyMMdd-hhmmss");

        configManager->copyDirectory(
            configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()),
            (configManager->getOldSaveDirectory(MainWindow::savesOrExtdata()) / std::filesystem::path((ui->downloadGameIDComboBox->currentText() + "-" + formattedDateTime).toStdString())));

        std::filesystem::remove_all(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));
        if (!std::filesystem::exists(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText())))
        {
            std::filesystem::create_directories(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));
        }

        int downloadResponse = citraholdServer->download(
            MainWindow::savesOrExtdata(),
            ui->downloadGameIDComboBox->currentText(),
            configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));

        if (downloadResponse)
        {
            showSuccessBox("Download successful!");
        }
        else
        {
            showErrorBox("Something went wrong with the download... " + downloadResponse);
        }
        return;
    }
}

void MainWindow::handleSaveExtdataRadios()
{
    // this is a particularly harsh way of stopping the user from accidentally sending extdata as saves or vice versa
    // it's not even that efficient

    if (!changedNameOrDirectorySinceSetAutomatically)
    {

        ui->directoryText->setPlainText(QString::fromStdString(configManager->getLikelyCitraDirectory(MainWindow::savesOrExtdata()).u8string()));
        ui->gameIDText->setPlainText("");
    }

    if (ui->stackedWidget->currentIndex() == 2)
    {
        handleServerFetch();
        configManager->updateConfigProperty("lastDownloadedType", savesOrExtdata() == UploadType::SAVES ? "SAVES" : "EXTDATA");
    }
}

void MainWindow::handleClearOldSavesButton()
{
    std::filesystem::remove_all(configManager->getOldSaveDirectory(MainWindow::savesOrExtdata()));
}

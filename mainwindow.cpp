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
        configManager->getConfig()["serverAddress"].toString(),
        configManager->getConfig()["token"].toString()
    );

    ui->setupUi(this);
    ui->statusbar->showMessage("Not logged in.");
    configManager = new ConfigManager();

    connect (
        ui->verifyButton, &QPushButton::clicked,
        this, &MainWindow::handleVerifyButtonClicked
    );

    connect (
        ui->tokenText, &QPlainTextEdit::textChanged,
        this, &MainWindow::manageTokenTextEdit
    );

    connect (ui->selectDirectoryButton, &QPushButton::clicked, this, [=](){
        handleDirectoryButton(true);
    });

    connect (ui->uploadButton, &QPushButton::clicked, this, &MainWindow::handleUploadButton);

    connect(ui->directoryText, &QPlainTextEdit::textChanged, this, [=](){
        handleDirectoryButton(false);
    });

    connect (ui->selectPreviousGameIDsButton, &QPushButton::clicked, this, &MainWindow::openGameIDSelector);
    connect (ui->saveGameIDButton, &QPushButton::clicked, this, &MainWindow::addGameIDToFile);
    connect (ui->gameIDText, &QPlainTextEdit::textChanged, this, [&]() {
        ui->saveGameIDButton->setDisabled(false);
    });

    connect (ui->extdataRadio, &QRadioButton::clicked, this, &MainWindow::handleSaveExtdataRadios);
    connect (ui->saveRadio, &QRadioButton::clicked, this, &MainWindow::handleSaveExtdataRadios);
    connect (ui->fetchButton, &QPushButton::clicked, this, &MainWindow::handleServerFetch);

    connect(ui->downloadButton, &QPushButton::clicked, this, &MainWindow::handleDownloadButton);

    // MainWindow.cpp (in the constructor)
    //connect(configManager, &ConfigManager::checkTokenInConfig, this, &MainWindow::handleTokenInConfig);

    QString userID = citraholdServer->verifyTokenToSetUserID(configManager->getToken());
    configManager->userID = userID;

    ui->directoryText->setPlainText(QString::fromStdString(configManager->getLikelyCitraDirectory().u8string()));

    if (configManager->loggedIn()) {
        ui->stackedWidget->setCurrentIndex(1);
        ui->statusbar->showMessage("Logged in.");

        handleServerFetch();

    } else {
        ui->stackedWidget->setCurrentIndex(0);
        // at some point, we'll need to check if the server is running
    }

    changedNameOrDirectorySinceSetAutomatically = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openGameIDSelector() {
    QDialog dialog;
    dialog.setWindowTitle("You must now select a game ID");

    // Create a layout for the dialog
    QVBoxLayout layout(&dialog);

    QJsonObject jsonObject = configManager->getGameIDFile(MainWindow::savesOrExtdata()).object();

    // Access the "gameID" key, which contains an array of arrays
    QJsonArray gameIDArray = jsonObject["gameID"].toArray();


    // Create a combo box (selector)
    QComboBox comboBox;

    QString ofWhich = (MainWindow::savesOrExtdata() == UploadType::SAVES ? "Saves" : "Extdata");
    comboBox.addItem("Select one please (" + ofWhich + ")");
    QVector<QString> gameIDPaths;
    // Iterate through the array of arrays and access the key-value pairs
    for (const QJsonValue& arrayValue : gameIDArray) {
        if (arrayValue.isArray()) {
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
    gameID.setMaximumHeight(30);  // Adjust the height as needed
    gameID.setMinimumHeight(30);
    gamePath.setLineWrapMode(QPlainTextEdit::NoWrap);
    gamePath.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameID.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gamePath.setMaximumHeight(30);  // Adjust the height as needed
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



    // Create an "OK" button
    QPushButton okButton("OK");
    layout.addWidget(&okButton);


    // Connect the "OK" button to a slot for processing




    gameID.setDisabled(true);
    gamePath.setDisabled(true);
    QObject::connect(&comboBox, &QComboBox::currentTextChanged, &dialog, [&]() {
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
        }
    });

    QObject::connect(&okButton, &QPushButton::clicked, &dialog, [&]() {
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
        for (int i = 1; i < comboBox.count(); i++) {
            if ((comboBox.itemText(i) != "") && (gameIDPaths[i - 1] != "")) {
                QJsonArray entry;
                entry.append(comboBox.itemText(i).trimmed());
                entry.append(gameIDPaths[i - 1].trimmed());
                newGameIDArray.append(entry);
            }
        }

        newGameIDObject["gameID"] = newGameIDArray;
        configManager->updateGameIDFile(MainWindow::savesOrExtdata(), QJsonDocument(newGameIDObject));

        dialog.close();
    });

    QObject::connect(&gameID, &QPlainTextEdit::textChanged, &dialog, [&]() {
        if (gameID.toPlainText() != comboBox.currentText()) {
            comboBox.setItemText(comboBox.currentIndex(), gameID.toPlainText());
            gameID.moveCursor(QTextCursor::End);
        }
    });

    QObject::connect(&gamePath, &QPlainTextEdit::textChanged, &dialog, [&]() {
        if (gamePath.toPlainText() != gameIDPaths[comboBox.currentIndex() - 1]) {
            gameIDPaths[comboBox.currentIndex() - 1] = gamePath.toPlainText();
            gamePath.moveCursor(QTextCursor::End);
            label.setText(comboBox.currentText() + " points to " + gameIDPaths[comboBox.currentIndex() - 1]);
        }
    });



    // VALIDATE ANY PATHS

    dialog.exec();

}

void MainWindow::manageTokenTextEdit()
{
    if (ui->tokenText->toPlainText().length() > 36)
    {
        // stop the user from typing more than 36 characters
        ui->tokenText->setPlainText(ui->tokenText->toPlainText().left(36));
    }
}

void MainWindow::handleVerifyButtonClicked()
{
    // we need to make {token: token}
    QJsonObject data;

    ui->tokenText->setPlainText(ui->tokenText->toPlainText().trimmed());

    if (ui->tokenText->toPlainText().length() <= 6)
    { // Handle a shorthand token
        // )
        // that gives a full token
        QString fullToken = citraholdServer->getTokenFromShorthandToken(ui->tokenText->toPlainText());
        qDebug() << "here";
        qDebug() << fullToken;
        if (fullToken != "invalid")
        {

            // configManager->setToken(fullToken);

            // get user ID
            ui->statusbar->showMessage("Logged in.");
            QString userID = citraholdServer->verifyTokenToSetUserID(fullToken);
            if (userID != "invalid")
            {
                configManager->userID = userID;
                configManager->setToken(fullToken);
                ui->tokenText->setPlainText("");
                ui->tokenOutput->setText("Successfully authenticated.");

                ui->stackedWidget->setCurrentIndex(1);
            } else {
                ui->tokenOutput->setText("You have the wrong credentials.");
            }
        } else {
            ui->tokenOutput->setText("You have the wrong credentials.");
        }
    }
    else // full token
    {    // get user ID
        QString fullToken = ui->tokenText->toPlainText();
        QString userID = citraholdServer->verifyTokenToSetUserID(fullToken);
        if (userID != "invalid")
        {
            ui->statusbar->showMessage("Logged in.");

            configManager->setToken(fullToken);
            configManager->userID = userID;

            ui->tokenText->setPlainText("");

            ui->tokenOutput->setText("Successfully authenticated.");

            ui->stackedWidget->setCurrentIndex(1);
        } else {
            ui->tokenOutput->setText("You have the wrong credentials.");
        }
    }
}

void MainWindow::handleDirectoryButton(bool openSelection) {

    changedNameOrDirectorySinceSetAutomatically = true;
    QString directory;
    ui->saveGameIDButton->setDisabled(false);

    if (openSelection) {
        directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), ui->directoryText->toPlainText());
    } else {
        directory = ui->directoryText->toPlainText();
    }
    if (directory != "") {
        if (openSelection) {
            ui->directoryText->setPlainText(directory);
        }

        if (directory.contains("extdata")) {
            ui->extdataRadio->toggle();
        } else {
            ui->saveRadio->toggle();

            // gets the gameid
            // this is probably a really shitty way to do it but I'm trying my best
        }

        /*
        if (QDir(directory).exists()) {
            QStringList pieces = directory.split("/data/")[0].split("/");
            std::reverse(pieces.begin(), pieces.end());
            QString gameID = (pieces[1].isEmpty() ? "" : pieces[1]) + (pieces[0].isEmpty() ? "" : pieces[0]);


            // in the future we could check if this is already set, but for now, it's up to the user
            //ui->gameIDText->setPlainText(gameID);
        }*/
    }
}

void MainWindow::showErrorBox(QString error) {
    QMessageBox errorMessage;
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.setWindowTitle("Error");
    errorMessage.setText("An error has occurred.\n");
    if (error != "") {
        errorMessage.setText(errorMessage.text() + "\n" + error);
    }

    errorMessage.setStandardButtons(QMessageBox::Ok);

    // Show the error message dialog
    errorMessage.exec();
}

UploadType MainWindow::savesOrExtdata() {
    return (ui->extdataRadio->isChecked() ? UploadType::EXTDATA : UploadType::SAVES);
}

void MainWindow::handleUploadButton() {

    QString directory = ui->directoryText->toPlainText();

    // todo: check directory is valid

    QString gameID = ui->gameIDText->toPlainText();

    if (gameID.trimmed() == "") {
        showErrorBox("Invalid game ID");
        return;
    }

    // at some point, we will need to check gameID is valid

    QQueue<QDir> directoriesToVisit;
    directoriesToVisit.enqueue(directory);

    QQueue<QDir> allFilesFull;
    QQueue<QDir> allFilesRelative;


    QDateTime currentDateTime = QDateTime::currentDateTime();

    QString formattedDateTime = currentDateTime.toString("yyyyMMdd-hhmmss");

    while (!directoriesToVisit.isEmpty()) {
        QDir currentDir = directoriesToVisit.dequeue();
        QStringList entryList = currentDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString &entry : entryList) {
            const QString fullPath = currentDir.filePath(entry);
            if (QFileInfo(fullPath).isDir()) {
                // If it's a directory, enqueue it for later processing
                directoriesToVisit.enqueue(QDir(fullPath));
            } else {
                // It's a file, do something with it
                allFilesFull.enqueue(fullPath);

                QString relativePath = QDir(directory).relativeFilePath(fullPath);
                allFilesRelative.enqueue(relativePath);
            }
        }
    }

    QVector<int> responses;
    for (const QDir &filePath : allFilesFull) {
        QFile file(filePath.path());

        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();

            qDebug() << filePath.path() << allFilesRelative.first().path();

            QString base64Data = QString(fileData.toBase64());

            // CHPC means Citrahold PC, the one from the 3DS will have CHDS instead
            responses.append(citraholdServer->upload(MainWindow::savesOrExtdata(), gameID + "/" + allFilesRelative.first().path(), base64Data));

        } else {
            // Handle the case where the file couldn't be opened
        }

        allFilesRelative.removeFirst();
    }

    int responseCode = 0;
    for (int value : responses) {
        responseCode = value;
        if (value != 201) {
            break; // Exit the loop as soon as a non-201 value is encountered
        }
    }

    if (responseCode == 201) {
        MainWindow::showErrorBox("Upload successful! HTTP " + QString::number(responseCode));
    } else {
        MainWindow::showErrorBox("Something went wrong during upload, HTTP " + QString::number(responseCode));
    }


}

void MainWindow::addGameIDToFile() {

    QJsonDocument gameIDFile = configManager->getGameIDFile(MainWindow::savesOrExtdata());

    QJsonValue gameIDValue = gameIDFile["gameID"];



    // Check if it's an array before converting
    if (gameIDValue.isArray()) {
        QJsonArray newGameIDArray = gameIDValue.toArray();

        QString directory = ui->directoryText->toPlainText();

        // todo: check directory is valid

        QString gameID = ui->gameIDText->toPlainText();

        if ((gameID != "") && (directory != "")) {
            QJsonArray entry;
            entry.append(gameID.trimmed());
            entry.append(directory.trimmed());
            newGameIDArray.append(entry);

            QJsonObject newGameIDFile;
            newGameIDFile["gameID"] = newGameIDArray;
            configManager->updateGameIDFile(MainWindow::savesOrExtdata(), QJsonDocument(newGameIDFile));
            ui->saveGameIDButton->setDisabled(true);
        }
    }
}

void MainWindow::handleServerFetch() {
    ui->downloadGameIDComboBox->clear();
    citraholdServer->updateServerGameIDVariables();

    QVector<QString>* serverGameIDs = (MainWindow::savesOrExtdata() == UploadType::SAVES) ? &citraholdServer->serverGameIDSaves : &citraholdServer->serverGameIDExtdata;

    for (const QString &key : *serverGameIDs) {

        ui->downloadGameIDComboBox->addItem(key);
        qDebug() << "Key:" << key;

    }
}

void MainWindow::handleDownloadButton() {

    // ui->downloadGameIDComboBox->currentText()
    //
    QJsonArray gameIDsOnFile = configManager->getGameIDFile(MainWindow::savesOrExtdata())["gameID"].toArray();

    for (int i = 0; i < gameIDsOnFile.size(); ++i) {
        QJsonValue value = gameIDsOnFile.at(i);

        bool gameIDpresent = false;

        if (ui->downloadGameIDComboBox->currentText() == value[0].toString()) {
            gameIDpresent = true;
        }

        if (!gameIDpresent) {
            showErrorBox("You need to add " + ui->downloadGameIDComboBox->currentText() + " to your gameID list.");
        } else {

            QDateTime currentDateTime = QDateTime::currentDateTime();
            QString formattedDateTime = currentDateTime.toString("yyyyMMdd-hhmmss");

            configManager->copyDirectory(
                configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()),
                // clear oldSaveDirectory at some point
                (configManager->getOldSaveDirectory(MainWindow::savesOrExtdata()) / std::filesystem::path ((ui->downloadGameIDComboBox->currentText() + "-" + formattedDateTime).toStdString()))
            );

            std::filesystem::remove_all(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));
            if (!std::filesystem::exists(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText())))
            {
                std::filesystem::create_directories(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));
            }

            citraholdServer->download(
                MainWindow::savesOrExtdata(),
                ui->downloadGameIDComboBox->currentText(),
                configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText())
            );
        }
    }

}

void MainWindow::handleSaveExtdataRadios() {
    // this is a particularly harsh way of stopping the user from accidentally sending extdata as saves or vice versa

    if (!changedNameOrDirectorySinceSetAutomatically) {

        ui->directoryText->setPlainText(QString::fromStdString(configManager->getLikelyCitraDirectory(MainWindow::savesOrExtdata()).u8string()));
        ui->gameIDText->setPlainText("");
    }
    handleServerFetch();
}

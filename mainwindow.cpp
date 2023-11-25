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
#include <QDesktopServices>
#include <QDateTime>

#include "gameidmanager.h"
#include "about.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    configManager = new ConfigManager();

    citraholdServer = new CitraholdServer(
        configManager->getConfigProperty("serverAddress"),
        configManager->getConfigProperty("token"));

    ui->setupUi(this);

    gameIDManager = new GameIDManager(this, configManager);
    aboutWindow = new About(this);

    ui->statusbar->showMessage("Not logged in.");

    ui->selectionBox->hide();

    configManager = new ConfigManager();

    connect(
        ui->verifyButton, &QPushButton::clicked,
        this, &MainWindow::handleVerifyButtonClicked);

    connect(
        ui->tokenText, &QPlainTextEdit::textChanged,
        this, &MainWindow::manageTokenTextEdit);

    connect(ui->uploadButton, &QPushButton::clicked, this, &MainWindow::handleUploadButton);

    connect(ui->selectPreviousGameIDsButton, &QPushButton::clicked, this, &MainWindow::openGameIDSelector);

    connect(ui->extdataRadio, &QRadioButton::clicked, this, &MainWindow::handleSaveExtdataRadios);
    connect(ui->saveRadio, &QRadioButton::clicked, this, &MainWindow::handleSaveExtdataRadios);
    connect(ui->fetchButton, &QPushButton::clicked, this, &MainWindow::handleServerFetch);

    connect(ui->downloadButton, &QPushButton::clicked, this, &MainWindow::handleDownloadButton);

    connect(ui->clearOldSavesButton, &QPushButton::clicked, this, &MainWindow::handleClearOldSavesButton);

    connect(ui->switchModeButton, &QPushButton::clicked, this, &MainWindow::handleToggleModeButton);

    connect(ui->aboutCitraholdOption, &QAction::triggered, this, &MainWindow::openAboutWindow);
    connect(ui->openConfigFolderOption, &QAction::triggered, this, &MainWindow::openConfigFolder);
    connect(ui->aboutCitraholdOption, &QAction::triggered, this, &MainWindow::openAboutWindow);

    connect(ui->uploadGameIDComboBox, &QComboBox::currentTextChanged, this, [&]
            { MainWindow::changeLastUploadText ();
            MainWindow::changeLastServerUploadText(true); });

    connect(ui->downloadGameIDComboBox, &QComboBox::currentTextChanged, this, [&]
            {
        MainWindow::changeLastDownloadText();
        MainWindow::changeLastServerUploadText(false); });

    QString userID = citraholdServer->verifyTokenToSetUserID(configManager->getToken());
    configManager->userID = userID;

    if (configManager->loggedIn())
    {

        handleSuccessfulLogin();
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->loginWidget);
        ui->selectionBox->hide();
        ui->citraholdLogo->show();
        // this is just because it's hidden in the designer
        ui->citraholdLogo->move(ui->citraholdLogo->x(), 30);
    }

    if (!citraholdServer->checkServerIsOnline())
    {
        showErrorBox("The server is inaccessible at this time. Please try again later.");
    }
    else
    {
        QStringList versions = citraholdServer->getSoftwareVersions();

        if (!versions.contains("v" + QCoreApplication::applicationVersion()) && (configManager->getConfigProperty("ignoreVersion") != versions[0]))
        {
            QMessageBox msgBox;
            QString message = "There is a new version of Citrahold PC available (" + versions[0] + "). Would you like to download it?";
            msgBox.setText(tr(message.toStdString().c_str()));
            QAbstractButton *pButtonYes = msgBox.addButton(tr("Yes"), QMessageBox::YesRole);
            msgBox.addButton(tr("Not now"), QMessageBox::NoRole);
            QAbstractButton *pButtonIgnore = msgBox.addButton(tr("Don't remind me"), QMessageBox::NoRole);

            msgBox.exec();

            if (msgBox.clickedButton() == pButtonYes)
            {
                QDesktopServices::openUrl(QUrl("https://github.com/regimensocial/citraholdUI/releases/latest"));
            }
            else if (msgBox.clickedButton() == pButtonIgnore)
            {
                configManager->updateConfigProperty("ignoreVersion", versions[0]);
            }
        }
    }

    retrieveGameIDList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeLastUploadText()
{
    QDateTime lastUploadTime = configManager->getLastUploadTime(MainWindow::savesOrExtdata(), ui->uploadGameIDComboBox->currentText());

    if (lastUploadTime.toString("yyyy") == "1970" || lastUploadTime.toString("yyyy") == "")
    {
        ui->lastDeviceUploadLabel->setText("");
    }
    else
    {
        ui->lastDeviceUploadLabel->setText("Last upload on this device: " + lastUploadTime.toString("dd/MM/yyyy hh:mm:ss"));
        ui->lastDeviceUploadLabel->setToolTip("day/month/year hour:minute:second");
    }
}

void MainWindow::changeLastDownloadText()
{
    QDateTime lastDownloadTime = configManager->getLastDownloadTime(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText());

    if (lastDownloadTime.toString("yyyy") == "1970" || lastDownloadTime.toString("yyyy") == "")
    {
        ui->lastDeviceDownloadLabel->setText("");
    }
    else
    {
        ui->lastDeviceDownloadLabel->setText("Last download on this device: " + lastDownloadTime.toString("dd/MM/yyyy hh:mm:ss"));
        ui->lastDeviceDownloadLabel->setToolTip("day/month/year hour:minute:second");
    }
}

void MainWindow::openGameIDSelector()
{
    gameIDManager->setModal(true);
    gameIDManager->setHidden(false);
    gameIDManager->exec();
}

void MainWindow::setUploadType(UploadType uploadType, bool ignoreOther)
{
    if (uploadType == UploadType::SAVES)
    {
        ui->saveRadio->setChecked(true);
        ui->extdataRadio->setChecked(false);
    }
    else if (uploadType == UploadType::EXTDATA)
    {
        ui->saveRadio->setChecked(false);
        ui->extdataRadio->setChecked(true);
    }

    retrieveGameIDList();

    if (!ignoreOther)
    {
        gameIDManager->setUploadType(uploadType, true);
    }
}

void MainWindow::handleToggleModeButton()
{
    if (ui->stackedWidget->currentIndex() == 1)
    {
        ui->stackedWidget->setCurrentWidget(ui->downloadWidget);
        ui->switchModeButton->setText("Switch to Upload");
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->uploadWidget);
        ui->switchModeButton->setText("Switch to Download");
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
    ui->stackedWidget->setCurrentWidget(ui->uploadWidget);
    ui->selectionBox->show();
    ui->citraholdLogo->hide();
    ui->statusbar->showMessage("Logged in.");
    handleServerFetch();

    QJsonObject config = configManager->getConfig().object();

    if (configManager->getConfigProperty("lastMode") == "DOWNLOAD")
    {
        handleToggleModeButton();
    }

    if (configManager->getConfigProperty("lastType") == "SAVES")
    {
        setUploadType(UploadType::SAVES);
    }
    else
    {

        setUploadType(UploadType::EXTDATA);
    }

    if ((configManager->getConfigProperty("lastDownloadedGameID") != "") && (ui->downloadGameIDComboBox->findText(configManager->getConfigProperty("lastDownloadedGameID")) != -1))
    {
        ui->downloadGameIDComboBox->setCurrentText(configManager->getConfigProperty("lastDownloadedGameID"));
    }

    if ((configManager->getConfigProperty("lastUploadedGameID") != "") && (ui->uploadGameIDComboBox->findText(configManager->getConfigProperty("lastUploadedGameID")) != -1))
    {
        ui->uploadGameIDComboBox->setCurrentText(configManager->getConfigProperty("lastUploadedGameID"));
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

void MainWindow::retrieveGameIDList(QString gameID)
{
    QJsonDocument gameIDFile = configManager->getGameIDFile(savesOrExtdata());

    QJsonArray gameIDArray = gameIDFile["gameID"].toArray();

    QString previousGameID = ui->uploadGameIDComboBox->currentText();

    ui->uploadGameIDComboBox->clear();

    for (int i = 0; i < gameIDArray.size(); ++i)
    {
        QJsonArray gameIDEntry = gameIDArray[i].toArray();
        ui->uploadGameIDComboBox->addItem(gameIDEntry[0].toString());
    }

    if (gameID != "")
    {
        previousGameID = gameID;
    }

    if ((previousGameID != "") && (ui->uploadGameIDComboBox->findText(previousGameID) != -1))
    {
        ui->uploadGameIDComboBox->setCurrentText(previousGameID);
    }

    ui->uploadButton->setEnabled(ui->uploadGameIDComboBox->count() > 0);
}

void MainWindow::handleUploadButton()
{

    ui->uploadButton->setEnabled(false);
    QString gameID = ui->uploadGameIDComboBox->currentText();
    QString directory = configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), gameID).string().c_str();

    if (gameID.trimmed() == "")
    {
        showErrorBox("Invalid game ID");
        ui->uploadButton->setEnabled(true);
        return;
    }

    QDateTime lastUploadTime = configManager->getLastUploadTime(MainWindow::savesOrExtdata(), gameID);
    QDateTime lastDownloadTime = configManager->getLastDownloadTime(MainWindow::savesOrExtdata(), gameID);
    QDateTime lastServerUploadTime = citraholdServer->getLastUploadTime(MainWindow::savesOrExtdata(), gameID);

    if (
        (lastUploadTime.toString("yyyy") != "1970" && lastUploadTime.toString("yyyy") != "") && 
        (lastServerUploadTime.toString("yyyy") != "1970" && lastServerUploadTime.toString("yyyy") != "") && 
        (lastDownloadTime.toString("yyyy") != "1970" && lastDownloadTime.toString("yyyy") != "")
    )
    {   
        if (lastServerUploadTime > lastDownloadTime) {
            QMessageBox msgBox;
            QString message = "You have uploaded to the server on a different device, but not downloaded it here.\nIt will overwrite whatever is on the server.\nAre you sure you want to upload?";
            msgBox.setText(tr(message.toStdString().c_str()));
            QAbstractButton *pButtonYes = msgBox.addButton(tr("Yes"), QMessageBox::YesRole);
            msgBox.addButton(tr("No"), QMessageBox::NoRole);

            msgBox.exec();

            if (msgBox.clickedButton() != pButtonYes)
            {
                ui->uploadButton->setEnabled(true);
                return;
            }
        }
    }

    QQueue<QDir> directoriesToVisit;
    directoriesToVisit.enqueue(directory);

    QQueue<QDir> allFilesFull;
    QQueue<QDir> allFilesRelative;

    while (!directoriesToVisit.isEmpty())
    {
        QDir currentDir = directoriesToVisit.dequeue();

        QStringList entryList = currentDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString &entry : entryList)
        {
            const QString fullPath = currentDir.filePath(entry);
            if (QFileInfo(fullPath).isDir())
            {
                directoriesToVisit.enqueue(QDir(fullPath));

                QString relativePath = QDir(directory).relativeFilePath(fullPath) + "/" + "citraholdDirectoryDummy";
                allFilesRelative.enqueue(relativePath);
            }
            else
            {
                allFilesFull.enqueue(fullPath);

                QString relativePath = QDir(directory).relativeFilePath(fullPath);
                allFilesRelative.enqueue(relativePath);
            }
        }
    }

    QJsonArray allFilesArray;

    int fileSizes = 0;

    for (const QDir &filePath : allFilesFull)
    {
        QFile file(filePath.path());

        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray fileData = file.readAll();

            fileSizes += fileData.size();

            file.close();

            // 128 MB limit, but this might be different on the server
            // TODO: make this configurable??????
            if (fileSizes > 128000000)
            {
                showErrorBox("Your save data is too large to upload.");
                ui->uploadButton->setEnabled(true);
                return;
            }

            QString base64Data = QString(fileData.toBase64());

            QJsonArray fileEntry;
            fileEntry.append(gameID + "/" + allFilesRelative.first().path());
            fileEntry.append(base64Data);

            allFilesArray.append(fileEntry);
        }
        else
        {
            qDebug() << "Failed to open file.";

            ui->uploadButton->setEnabled(true);
        }

        allFilesRelative.removeFirst();
    }

    qDebug() << "Total file size: " << fileSizes;
    int responseCode = citraholdServer->uploadMultiple(MainWindow::savesOrExtdata(), allFilesArray);
    qDebug() << "Response code: " << responseCode;

    if (responseCode == 201)
    {
        MainWindow::showSuccessBox("Upload successful!");

        configManager->updateConfigProperty("lastType", (savesOrExtdata() == UploadType::SAVES ? "SAVES" : "EXTDATA"));
        configManager->updateConfigProperty("lastUploadedGameID", ui->uploadGameIDComboBox->currentText());
        configManager->updateConfigProperty("lastMode", "UPLOAD");

        QDateTime currentDateTime = citraholdServer->getLastUploadTime(MainWindow::savesOrExtdata(), ui->uploadGameIDComboBox->currentText());
        configManager->setLastUploadTime(MainWindow::savesOrExtdata(), ui->uploadGameIDComboBox->currentText(), currentDateTime);
        configManager->setLastDownloadTime(MainWindow::savesOrExtdata(), ui->uploadGameIDComboBox->currentText(), currentDateTime.addSecs(1));
        changeLastUploadText();

        handleServerFetch();
    }
    else
    {
        showErrorBox("Something went wrong during upload, HTTP " + QString::number(responseCode));
    }

    ui->uploadButton->setEnabled(true);
}

void MainWindow::handleServerFetch()
{
    QString previousGameID = ui->downloadGameIDComboBox->currentText();

    ui->downloadGameIDComboBox->clear();
    citraholdServer->updateServerGameIDVariables();

    QVector<QString> *serverGameIDs = (MainWindow::savesOrExtdata() == UploadType::SAVES) ? &citraholdServer->serverGameIDSaves : &citraholdServer->serverGameIDExtdata;

    ui->downloadButton->setEnabled((serverGameIDs->size() > 0));

    for (const QString &key : *serverGameIDs)
    {
        ui->downloadGameIDComboBox->addItem(key);
    }

    if (ui->downloadGameIDComboBox->findText(previousGameID) != -1)
    {
        ui->downloadGameIDComboBox->setCurrentText(previousGameID);
    }

    changeLastServerUploadText(true);

    if (ui->uploadGameIDComboBox->currentText() == ui->downloadGameIDComboBox->currentText())
    {
        changeLastServerUploadText(false);
    }
}

void MainWindow::handleDownloadGameIDMissing()
{
    showErrorBox("You need to add " + ui->downloadGameIDComboBox->currentText().trimmed() + " to your gameID list.");
    gameIDManager->setGameID(ui->downloadGameIDComboBox->currentText());
    openGameIDSelector();
}

void MainWindow::handleDownloadButton()
{
    ui->downloadButton->setEnabled(false);
    QJsonArray gameIDsOnFile = configManager->getGameIDFile(MainWindow::savesOrExtdata())["gameID"].toArray();

    if (gameIDsOnFile.size() == 0)
    {
        handleDownloadGameIDMissing();
        ui->downloadButton->setEnabled(true);
        return;
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
        handleDownloadGameIDMissing();
        ui->downloadButton->setEnabled(true);
        return;
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

        int downloadResponse = citraholdServer->downloadMultiple(
            MainWindow::savesOrExtdata(),
            ui->downloadGameIDComboBox->currentText(),
            configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));

        if (downloadResponse)
        {
            showSuccessBox("Download successful!");

            configManager->updateConfigProperty("lastType", (savesOrExtdata() == UploadType::SAVES ? "SAVES" : "EXTDATA"));
            configManager->updateConfigProperty("lastDownloadedGameID", ui->downloadGameIDComboBox->currentText());
            configManager->updateConfigProperty("lastMode", "DOWNLOAD");
            configManager->setLastDownloadTime(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText(), QDateTime::currentDateTime());
            changeLastServerUploadText(false);

            if (ui->uploadGameIDComboBox->currentText() == ui->downloadGameIDComboBox->currentText())
            {
                changeLastServerUploadText(true);
            }

            changeLastDownloadText();
        }
        else
        {
            showErrorBox("Something went wrong with the download... ");

            std::filesystem::remove_all(configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));
            configManager->copyDirectory(
                (configManager->getOldSaveDirectory(MainWindow::savesOrExtdata()) / std::filesystem::path((ui->downloadGameIDComboBox->currentText() + "-" + formattedDateTime).toStdString())),
                configManager->getGamePathFromGameID(MainWindow::savesOrExtdata(), ui->downloadGameIDComboBox->currentText()));
        }

        ui->downloadButton->setEnabled(true);
        return;
    }
}

void MainWindow::handleSaveExtdataRadios()
{
    setUploadType(savesOrExtdata());

    if (ui->stackedWidget->currentIndex() == 2)
    {
        handleServerFetch();
    }
}

void MainWindow::handleClearOldSavesButton()
{
    std::filesystem::remove_all(configManager->getOldSaveDirectory(MainWindow::savesOrExtdata()));
}

void MainWindow::openAboutWindow()
{
    aboutWindow->exec();
}

void MainWindow::openConfigFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(configManager->getSaveDirectory().string().c_str()));
}

void MainWindow::changeLastServerUploadText(bool upload)
{

    QComboBox *comboBox = upload ? ui->uploadGameIDComboBox : ui->downloadGameIDComboBox;
    QLabel *label = upload ? ui->uploadLastUploadedLabel : ui->downloadLastUploadedLabel;

    QString gameID = comboBox->currentText();

    if (gameID == "")
    {
        return;
    }

    QDateTime lastUploadTime = citraholdServer->getLastUploadTime(MainWindow::savesOrExtdata(), gameID);

    if (lastUploadTime.toString("yyyy") == "1970" || lastUploadTime.toString("yyyy") == "")
    {
        label->setText("");
    }
    else
    {
        label->setText("Last upload on server: " + lastUploadTime.toString("dd/MM/yyyy hh:mm:ss"));
        label->setToolTip("day/month/year hour:minute:second");
    }
}

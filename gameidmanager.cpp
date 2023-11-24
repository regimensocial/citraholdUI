#include "gameidmanager.h"
#include "ui_gameidmanager.h"
#include "ConfigManager.h"
#include "CitraholdServer.h"
#include "mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

GameIDManager::GameIDManager(QWidget *parent, ConfigManager *configManager) : QDialog(parent), ui(new Ui::GameIDManager)
{
    ui->setupUi(this);

    this->configManager = configManager;

    connect(ui->resetDirectoryButton, &QPushButton::clicked, this, &GameIDManager::resetDirectory);
    connect(ui->extdataRadio, &QRadioButton::clicked, this, &GameIDManager::handleSaveExtdataRadios);
    connect(ui->saveRadio, &QRadioButton::clicked, this, &GameIDManager::handleSaveExtdataRadios);
    connect(ui->selectDirectoryButton, &QPushButton::clicked, this, [&]
            { handleDirectoryButton(true); });
    connect(ui->directoryText, &QPlainTextEdit::textChanged, this, [&]
            { handleDirectoryButton(false); });
    connect(ui->gameIDText, &QPlainTextEdit::textChanged, this, &GameIDManager::handleChangeToNew);
    connect(ui->addGameIDButton, &QPushButton::clicked, this, &GameIDManager::addGameIDToFile);
    connect(ui->switchModeButton, &QPushButton::clicked, this, &GameIDManager::switchBetweenNewAndExisting);
    connect(ui->existingDirectoryText, &QPlainTextEdit::textChanged, this, &GameIDManager::handleChangeToExisting);
    connect(ui->existingGameIDText, &QPlainTextEdit::textChanged, this, &GameIDManager::handleChangeToExisting);
    connect(ui->gameIDComboBox, &QComboBox::currentTextChanged, this, &GameIDManager::handleExistingGameIDSelection);
    connect(ui->updateGameIDButton, &QPushButton::clicked, this, [&]
            { addGameIDToFile(true); });
    connect(ui->existingDirectoryButton, &QPushButton::clicked, this, [&]
            { handleDirectoryButton(true, true); });
    connect(ui->deleteGameIDButton, &QPushButton::clicked, this, &GameIDManager::handleDeleteExistingGameID);
    connect(ui->openDirectoryButton, &QPushButton::clicked, this, [&]
            { QDesktopServices::openUrl(QUrl::fromLocalFile(ui->existingDirectoryText->toPlainText())); });

    resetDirectory();
    ui->addGameIDButton->setDisabled(true);
    ui->stackedWidget->setCurrentIndex(0);

    this->setHidden(true);
}

GameIDManager::~GameIDManager()
{
    delete ui;
}

void GameIDManager::resetDirectory(bool doubleCheck)
{

    if (!doubleCheck || (doubleCheck && ((ui->directoryText->toPlainText().isEmpty() || ui->directoryText->toPlainText() == QString::fromStdString(this->configManager->getLikelyCitraDirectory(UploadType::SAVES).u8string()) || ui->directoryText->toPlainText() == QString::fromStdString(this->configManager->getLikelyCitraDirectory(UploadType::EXTDATA).u8string())))))
    {
        ui->directoryText->setPlainText(QString::fromStdString(this->configManager->getLikelyCitraDirectory(uploadType).u8string()));
    }
}

void GameIDManager::setGameID(QString gameID)
{
    ui->gameIDText->setPlainText(gameID);
    if (ui->stackedWidget->currentIndex() == 1)
    {
        switchBetweenNewAndExisting();
    }
}

void GameIDManager::setUploadType(UploadType uploadType, bool ignoreOther)
{
    qDebug() << "Setting upload type to " << uploadType;
    this->uploadType = uploadType;
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

    if (ui->stackedWidget->currentIndex() == 1)
    {
        retrieveGameIDList();
    }

    if (!ignoreOther)
    {
        ((MainWindow *)parentWidget())->setUploadType(uploadType);
    }
    resetDirectory(true);
}

void GameIDManager::handleSaveExtdataRadios()
{
    if (
        !this->isHidden() && ((
                                  !ui->directoryText->toPlainText().isEmpty() &&
                                  ui->directoryText->toPlainText() != QString::fromStdString(this->configManager->getLikelyCitraDirectory(UploadType::SAVES).u8string()) &&
                                  ui->directoryText->toPlainText() != QString::fromStdString(this->configManager->getLikelyCitraDirectory(UploadType::EXTDATA).u8string())) &&
                              ui->directoryText->toPlainText().contains("extdata") && uploadType == UploadType::EXTDATA))
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Extdata Detected", "This might be extdata. Would you like to switch to extdata mode?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            setUploadType(UploadType::EXTDATA);
        }
    }

    if (ui->saveRadio->isChecked())
    {
        setUploadType(UploadType::SAVES);
    }
    else if (ui->extdataRadio->isChecked())
    {
        setUploadType(UploadType::EXTDATA);
    }
}

void GameIDManager::handleDirectoryButton(bool openSelection, bool existing)
{

    QString directory = existing ? ui->existingDirectoryText->toPlainText() : ui->directoryText->toPlainText();

    if (openSelection)
    {
        qDebug() << "Opening directory selection";
        directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), directory);
    }

    if (directory != "")
    {
        // check if directory string contains "extdata"

        if (!this->isHidden() && !existing && directory.contains("extdata") && uploadType != UploadType::EXTDATA)
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Extdata Detected", "This might be extdata. Would you like to switch to extdata mode?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                setUploadType(UploadType::EXTDATA);
            }
        }

        (existing) ? handleChangeToExisting() : handleChangeToNew();

        if (openSelection)
        {
            if (existing)
            {
                (ui->existingDirectoryText)->setPlainText(directory);
            }
            else
            {
                (ui->directoryText)->setPlainText(directory);
            }
        }
    }
}

void GameIDManager::handleChangeToNew()
{
    bool validDirectory = (ui->directoryText->toPlainText().trimmed() != "" &&
                           std::filesystem::exists(ui->directoryText->toPlainText().toStdString()) &&
                           ui->directoryText->toPlainText().trimmed().toStdString() != this->configManager->getLikelyCitraDirectory(uploadType).u8string());

    if (
        ui->gameIDText->toPlainText().trimmed() != "" &&
        validDirectory)
    {
        ui->addGameIDButton->setDisabled(false);
    }
    else
    {
        ui->addGameIDButton->setDisabled(true);
    }
}

void GameIDManager::addGameIDToFile(bool existing)
{
    QString gameID;
    QString directory;

    if (!existing)
    {
        gameID = ui->gameIDText->toPlainText();
        directory = ui->directoryText->toPlainText();
    }
    else
    {
        gameID = ui->existingGameIDText->toPlainText();
        directory = ui->existingDirectoryText->toPlainText();
    }

    if (!std::filesystem::exists(directory.toStdString()))
    {
        QMessageBox::critical(this, "Error", "Directory does not exist.");
    }

    if (!existing)
    {
        if (configManager->getGamePathFromGameID(uploadType, gameID) != "")
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Duplicate Game ID", "You're trying to add a Game ID which already exists. Do you want to overwrite it?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                configManager->removeEntryFromGameIDFile(uploadType, gameID);
            }
            else
            {
                return;
            }
        }
    }
    else
    {
        configManager->removeEntryFromGameIDFile(uploadType, gameID);
        configManager->removeEntryFromGameIDFile(uploadType, ui->gameIDComboBox->currentText());
    }

    this->configManager->addEntryToGameIDFile(uploadType, gameID, directory);

    QString message = gameID + " successfully " + (existing ? "updated in " : "added to ") + (uploadType == UploadType::SAVES ? "saves" : "extdata") + ".";
    QMessageBox::information(this, "Success", message);

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Close Game ID Menu", "Should Game ID Menu be closed now?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        this->close();
    }

    ui->gameIDText->setPlainText("");
    resetDirectory(false);

    retrieveGameIDList();
    ((MainWindow *)parentWidget())->retrieveGameIDList(gameID);

    if (existing)
    {
        ui->gameIDComboBox->setCurrentText(gameID);
    }
}

void GameIDManager::switchBetweenNewAndExisting()
{
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() == 0 ? 1 : 0);
    if (ui->stackedWidget->currentIndex() == 0)
    {
        ui->activityLabel->setText("You are adding a new game ID");
        ui->switchModeButton->setText("Switch to existing Game IDs");
    }
    else
    {
        ui->activityLabel->setText("You are looking at existing game IDs");
        ui->switchModeButton->setText("Switch to adding a new Game ID");
        retrieveGameIDList();
    }
}

void GameIDManager::retrieveGameIDList()
{
    QJsonDocument gameIDFile = configManager->getGameIDFile(uploadType);

    QJsonArray gameIDArray = gameIDFile["gameID"].toArray();

    QString previousGameID = ui->gameIDComboBox->currentText();

    ui->gameIDComboBox->clear();

    for (int i = 0; i < gameIDArray.size(); ++i)
    {
        QJsonArray gameIDEntry = gameIDArray[i].toArray();
        ui->gameIDComboBox->addItem(gameIDEntry[0].toString());
    }

    if ((previousGameID != "") && (ui->gameIDComboBox->findText(previousGameID) != -1))
    {
        ui->gameIDComboBox->setCurrentText(previousGameID);
    }

    ui->deleteGameIDButton->setDisabled(ui->gameIDComboBox->count() == 0);
    ui->existingDirectoryButton->setDisabled(ui->gameIDComboBox->count() == 0);
    ui->existingDirectoryText->setDisabled(ui->gameIDComboBox->count() == 0);
    ui->existingGameIDText->setDisabled(ui->gameIDComboBox->count() == 0);
    ui->openDirectoryButton->setDisabled(ui->gameIDComboBox->count() == 0);

    handleExistingGameIDSelection();
}

void GameIDManager::handleExistingGameIDSelection()
{
    QString gameID = ui->gameIDComboBox->currentText();
    std::filesystem::path directory = configManager->getGamePathFromGameID(uploadType, gameID);

    ui->existingGameIDText->setPlainText(gameID);
    ui->existingDirectoryText->setPlainText(directory.string().c_str());
    ui->updateGameIDButton->setDisabled(true);
}

void GameIDManager::handleChangeToExisting()
{
    bool validDirectory = (ui->existingDirectoryText->toPlainText().trimmed() != "" &&
                           ui->existingDirectoryText->toPlainText() != ui->directoryText->toPlainText() &&
                           std::filesystem::exists(ui->existingDirectoryText->toPlainText().toStdString()));
    
    ui->openDirectoryButton->setDisabled(!validDirectory);

    if (
        (
            ui->existingGameIDText->toPlainText().trimmed() != "" &&
            ui->existingGameIDText->toPlainText() != ui->gameIDComboBox->currentText()) ||
        (std::filesystem::exists(ui->existingDirectoryText->toPlainText().toStdString())))
    {
        ui->updateGameIDButton->setDisabled(false);
    }
    else
    {
        ui->updateGameIDButton->setDisabled(true);
    }
}

void GameIDManager::handleDeleteExistingGameID()
{
    QString gameID = ui->gameIDComboBox->currentText();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete Game ID", "Are you sure you want to delete " + gameID + " from " + (uploadType == UploadType::EXTDATA ? "extdata?" : "saves?"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        configManager->removeEntryFromGameIDFile(uploadType, gameID);
        retrieveGameIDList();
    }
}

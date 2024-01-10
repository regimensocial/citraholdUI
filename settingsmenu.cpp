#include "settingsmenu.h"
#include "ui_settingsmenu.h"
#include <QAbstractSocket>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkInterface>

SettingsMenu::SettingsMenu(QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsMenu) {
    ui->setupUi(this);

    connect(ui->roleComboBox, &QComboBox::currentTextChanged, this,
            &SettingsMenu::roleComboBoxChange);

    // for now, later we will load the settings from a config file
    ui->roleComboBox->setCurrentIndex(0);
    ui->stackedWidget->setCurrentWidget(ui->clientToCentralServerPage);

    // test what this does when offline
    
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != localhost) {
            ui->privateIPLabel->setText("Your private IP is likely " +
                                        address.toString() + ".");
        }
    }

    // privateIPLabel
}

// general

void SettingsMenu::handleSaveButton() {}

void SettingsMenu::handleCancelButton() {}

void SettingsMenu::roleComboBoxChange() {
    int currentIndex = ui->roleComboBox->currentIndex();

    if (currentIndex == 0) {
        ui->stackedWidget->setCurrentWidget(ui->clientToCentralServerPage);
    } else if (currentIndex == 1) {
        ui->stackedWidget->setCurrentWidget(ui->localServerPage);
    } else if (currentIndex == 2) {
        ui->stackedWidget->setCurrentWidget(ui->clientToLocalServerPage);
    }
}

// client to central server

void SettingsMenu::handleCentralServerAddressChange() {}

void SettingsMenu::handleResetValuesButton() {}

void SettingsMenu::handleVerifyCentralServerButton() {}

// client to local server

void SettingsMenu::handleAutodetectLocalServerButton() {}

void SettingsMenu::handleLocalServerAddressChange() {}

void SettingsMenu::handleVerifyLocalServerButton() {}

// server hosting

void SettingsMenu::handleLocalServerPortChange() {}

void SettingsMenu::handleLocalServerToggle() {}

void SettingsMenu::handleLocalServerTestButton() {}

void SettingsMenu::handleLocalServerPortResetButton() {}

void SettingsMenu::handlePromptDownloadCheckbox() {}

SettingsMenu::~SettingsMenu() { delete ui; }

#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H

#include <QDialog>
#include "InternalServer.h"

namespace Ui {
class SettingsMenu;
}

class SettingsMenu : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsMenu(QWidget *parent = nullptr, InternalServer *internalServer = nullptr);
    ~SettingsMenu();

private slots:

    // general
    void handleSaveButton();
    void handleCancelButton();
    void roleComboBoxChange();

    // client to central server
    void handleCentralServerAddressChange();
    void handleResetValuesButton();
    void handleVerifyCentralServerButton();

    // client to local server
    void handleAutodetectLocalServerButton();
    void handleLocalServerAddressChange();
    void handleVerifyLocalServerButton();

    // server hosting
    void handleLocalServerPortChange();
    void handleLocalServerToggle();
    void handleLocalServerTestButton();
    void handleLocalServerPortResetButton();
    void handlePromptDownloadCheckbox();

private:
    Ui::SettingsMenu *ui;
    InternalServer *internalServer;
    bool areChangesVerified = false;
};

#endif // SETTINGSMENU_H

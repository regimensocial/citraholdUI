#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ConfigManager.h"
#include <QNetworkAccessManager>
#include "CitraholdServer.h"
#include "InternalServer.h"
#include "gameidmanager.h"
#include "settingsmenu.h"
#include "about.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setUploadType(UploadType uploadType, bool ignoreOther = false);
    void retrieveGameIDList(QString gameID = "");

private slots:
    void handleVerifyButtonClicked();
    void manageTokenTextEdit();

    void handleUploadButton();
    void handleServerFetch();
    void handleDownloadButton();
    void handleClearOldSavesButton();
    void handleToggleModeButton();
    void openConfigFolder();
    void openAboutWindow();
    void changeLastUploadText();
    void changeLastDownloadText();
    void changeLastServerUploadText(bool upload);
    void openSettings();

private:
    Ui::MainWindow *ui;
    ConfigManager *configManager;
    CitraholdServer *citraholdServer;
    GameIDManager *gameIDManager;
    SettingsMenu *settingsMenu;
    About *aboutWindow;
    InternalServer *internalServer;

    void handleSuccessfulLogin();
    void openGameIDSelector();
    void handleSaveExtdataRadios();
    void handleDownloadGameIDMissing();
    void showErrorBox(QString error = "");
    void showSuccessBox(QString text);

    UploadType savesOrExtdata();

};
#endif // MAINWINDOW_H

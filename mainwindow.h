#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ConfigManager.h"
#include <QNetworkAccessManager>
#include "CitraholdServer.h"
#include "gameidmanager.h"
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


private:
    Ui::MainWindow *ui;
    ConfigManager *configManager;
    CitraholdServer *citraholdServer;
    GameIDManager *gameIDManager;
    About *aboutWindow;

    void handleSuccessfulLogin();
    void openGameIDSelector();
    void handleSaveExtdataRadios();
    void handleDownloadGameIDMissing();
    void showErrorBox(QString error = "");
    void showSuccessBox(QString text);

    UploadType savesOrExtdata();

};
#endif // MAINWINDOW_H

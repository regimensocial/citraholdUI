#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ConfigManager.h"
#include <QNetworkAccessManager>
#include "CitraholdServer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleVerifyButtonClicked();
    void manageTokenTextEdit();

    void handleDirectoryButton(bool openSelection = true);
    void handleUploadButton();
    void handleServerFetch();
    void handleDownloadButton();
    void handleClearOldSavesButton();
    void handleToggleModeButton();

private:
    Ui::MainWindow *ui;
    ConfigManager *configManager;
    CitraholdServer *citraholdServer;

    void handleSuccessfulLogin();
    void openGameIDSelector();
    void addGameIDToFile();
    void showErrorBox(QString error = "");
    void showSuccessBox(QString text);
    UploadType savesOrExtdata();

    void handleSaveExtdataRadios();


    bool changedNameOrDirectorySinceSetAutomatically;

};
#endif // MAINWINDOW_H

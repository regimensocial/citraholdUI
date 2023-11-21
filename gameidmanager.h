#ifndef GAMEIDMANAGER_H
#define GAMEIDMANAGER_H

#include <QDialog>
#include "ConfigManager.h"
#include "CitraholdServer.h"

namespace Ui {
class GameIDManager;
}

class GameIDManager : public QDialog
{
    Q_OBJECT

public:
    explicit GameIDManager(QWidget *parent = nullptr, ConfigManager *configManager = nullptr);
    ~GameIDManager();

    ConfigManager *configManager;
    void setUploadType(UploadType uploadType, bool ignoreOther = false);
    void setGameID(QString gameID);

private:
    Ui::GameIDManager *ui;
    UploadType uploadType;

private slots:
    void resetDirectory(bool doubleCheck = false);
    void handleSaveExtdataRadios();
    void handleDirectoryButton(bool openSelection = true, bool existing = false);
    void addGameIDToFile(bool existing = false);
    void switchBetweenNewAndExisting();
    void retrieveGameIDList();
    void handleExistingGameIDSelection();
    void handleChangeToNew();
    void handleChangeToExisting();
    void handleDeleteExistingGameID();

};

#endif // GAMEIDMANAGER_H

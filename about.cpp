#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);
    ui->versionLabel->setText("This is Citrahold PC v" + QCoreApplication::applicationVersion());
}

About::~About()
{
    delete ui;
}

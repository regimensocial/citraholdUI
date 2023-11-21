QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CitraholdServer.cpp \
    about.cpp \
    gameidmanager.cpp \
    main.cpp \
    mainwindow.cpp \
    ConfigManager.cpp

HEADERS += \
    CitraholdServer.h \
    about.h \
    gameidmanager.h \
    mainwindow.h \
    ConfigManager.h

FORMS += \
    about.ui \
    gameidmanager.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    banner.png \
    icon.png

RESOURCES += \
    banner.qrc

TARGET = Citrahold
TEMPLATE = app

VERSION = 1.0.1

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

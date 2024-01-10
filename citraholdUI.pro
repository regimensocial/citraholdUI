QT       += core gui network widgets httpserver

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CitraholdServer.cpp \
    InternalServer.cpp \
    about.cpp \
    gameidmanager.cpp \
    main.cpp \
    mainwindow.cpp \
    ConfigManager.cpp \
    settingsmenu.cpp

HEADERS += \
    CitraholdServer.h \
    InternalServer.h \
    about.h \
    gameidmanager.h \
    mainwindow.h \
    ConfigManager.h \
    settingsmenu.h

FORMS += \
    about.ui \
    gameidmanager.ui \
    mainwindow.ui \
    settingsmenu.ui

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

VERSION = 1.1.1

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Select ARM build in Creator!!
QMAKE_APPLE_DEVICE_ARCHS += \
    x86_64 \
    arm64

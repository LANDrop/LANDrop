QT += core gui widgets network

CONFIG += c++11

SOURCES += \
    aboutdialog.cpp \
    crypto.cpp \
    discoveryservice.cpp \
    filetransferdialog.cpp \
    filetransferreceiver.cpp \
    filetransfersender.cpp \
    filetransferserver.cpp \
    filetransfersession.cpp \
    main.cpp \
    sendtodialog.cpp \
    settings.cpp \
    settingsdialog.cpp \
    trayicon.cpp

HEADERS += \
    aboutdialog.h \
    crypto.h \
    discoveryservice.h \
    filetransferdialog.h \
    filetransferreceiver.h \
    filetransfersender.h \
    filetransferserver.h \
    filetransfersession.h \
    sendtodialog.h \
    settings.h \
    settingsdialog.h \
    trayicon.h

FORMS += \
    aboutdialog.ui \
    filetransferdialog.ui \
    sendtodialog.ui \
    settingsdialog.ui

RESOURCES += \
    icons.qrc \
    locales.qrc

TRANSLATIONS += \
    locales/LANDrop.zh_CN.ts

RC_ICONS = icons/app.ico
ICON = icons/app.icns

unix {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lsodium
}

QMAKE_INFO_PLIST = Info.plist

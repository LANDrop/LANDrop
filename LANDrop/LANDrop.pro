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
    selectfilesdialog.cpp \
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
    selectfilesdialog.h \
    sendtodialog.h \
    settings.h \
    settingsdialog.h \
    trayicon.h

FORMS += \
    aboutdialog.ui \
    filetransferdialog.ui \
    selectfilesdialog.ui \
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
    LIBS += -lsodium
    INCLUDEPATH += /usr/include
    LIBS += -L/usr/lib

    PREFIX = $$(PREFIX)
    isEmpty(PREFIX) {
        PREFIX = /usr
    }

    binary.path = $$PREFIX/bin
    binary.files = $$OUT_PWD/landrop
    binary.extra = cp "$$OUT_PWD/LANDrop" "$$OUT_PWD/landrop"
    binary.CONFIG = no_check_exist executable

    icon.path = $$PREFIX/share/icons/hicolor/scalable/apps
    icon.files = $$OUT_PWD/landrop.svg
    icon.extra = cp "$$PWD/icons/app.svg" "$$OUT_PWD/landrop.svg"
    icon.CONFIG = no_check_exist 

    desktop.path = $$PREFIX/share/applications
    desktop.files = $$OUT_PWD/landrop.desktop
    desktop.extra = \
        cp "$$PWD/../misc/LANDrop.desktop" "$$OUT_PWD/landrop.desktop" && \
        sed -i 's/Exec=LANDrop/Exec=landrop/g' "$$OUT_PWD/landrop.desktop" && \
        sed -i 's/Icon=LANDrop/Icon=landrop/g' "$$OUT_PWD/landrop.desktop"
    desktop.CONFIG = no_check_exist 

    INSTALLS += binary icon desktop
}

QMAKE_INFO_PLIST = Info.plist

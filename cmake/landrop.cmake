set(LANDROP_SRC "${CMAKE_SOURCE_DIR}/LANDrop")
set(LANDROP_QRC
    ${LANDROP_SRC}/icons.qrc
    ${LANDROP_SRC}/locales.qrc
)

set(LANDROP_SOURCES
    ${LANDROP_SRC}/aboutdialog.cpp
    ${LANDROP_SRC}/crypto.cpp
    ${LANDROP_SRC}/discoveryservice.cpp
    ${LANDROP_SRC}/filetransferdialog.cpp
    ${LANDROP_SRC}/filetransferreceiver.cpp
    ${LANDROP_SRC}/filetransfersender.cpp
    ${LANDROP_SRC}/filetransferserver.cpp
    ${LANDROP_SRC}/filetransfersession.cpp
    ${LANDROP_SRC}/main.cpp
    ${LANDROP_SRC}/selectfilesdialog.cpp
    ${LANDROP_SRC}/sendtodialog.cpp
    ${LANDROP_SRC}/settings.cpp
    ${LANDROP_SRC}/settingsdialog.cpp
    ${LANDROP_SRC}/trayicon.cpp
)

set(LANDROP_HEADERS
    ${LANDROP_SRC}/aboutdialog.h
    ${LANDROP_SRC}/crypto.h
    ${LANDROP_SRC}/discoveryservice.h
    ${LANDROP_SRC}/filetransferdialog.h
    ${LANDROP_SRC}/filetransferreceiver.h
    ${LANDROP_SRC}/filetransfersender.h
    ${LANDROP_SRC}/filetransferserver.h
    ${LANDROP_SRC}/filetransfersession.h
    ${LANDROP_SRC}/selectfilesdialog.h
    ${LANDROP_SRC}/sendtodialog.h
    ${LANDROP_SRC}/settings.h
    ${LANDROP_SRC}/settingsdialog.h
    ${LANDROP_SRC}/trayicon.h
)

set(LANDROP_UI
    ${LANDROP_SRC}/aboutdialog.ui
    ${LANDROP_SRC}/filetransferdialog.ui
    ${LANDROP_SRC}/selectfilesdialog.ui
    ${LANDROP_SRC}/sendtodialog.ui
    ${LANDROP_SRC}/settingsdialog.ui
)

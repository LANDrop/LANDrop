#pragma once

#include <QMenu>
#include <QSystemTrayIcon>

#include "aboutdialog.h"
#include "discoveryservice.h"
#include "filetransferserver.h"
#include "settingsdialog.h"

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = nullptr);
private:
    QMenu menu;
    AboutDialog aboutDialog;
    SettingsDialog settingsDialog;
    FileTransferServer server;
    DiscoveryService discoveryService;
private slots:
    void sendActionTriggered();
    void exitActionTriggered();
    void trayIconActivated(ActivationReason reason);
};

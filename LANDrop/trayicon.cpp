/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, LANDrop
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QNetworkProxy>
#include <QTimer>
#include <QUrl>

#include "selectfilesdialog.h"
#include "settings.h"
#include "trayicon.h"

TrayIcon::TrayIcon(QObject *parent) : QSystemTrayIcon(parent)
{
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    QIcon appIcon(":/icons/app.png");
    QIcon appMaskIcon(":/icons/app_mask.png");
    appMaskIcon.setIsMask(true);
    QIcon sendIcon(":/icons/send.png");
    QIcon openDownloadFolderIcon(":/icons/open_download_folder.png");
    QIcon settingsIcon(":/icons/settings.png");
    QIcon aboutIcon(":/icons/about.png");
    QIcon exitIcon(":/icons/exit.png");
    if (QSysInfo::productType() == "osx" || QSysInfo::productType() == "macos")
        setIcon(appMaskIcon);
    else
        setIcon(appIcon);

    QAction *action, *addrPortAction;
    addrPortAction = menu.addAction("");
    addrPortAction->setEnabled(false);
    menu.addSeparator();
    action = menu.addAction(sendIcon, tr("Send File(s)..."));
    connect(action, &QAction::triggered, this, &TrayIcon::sendActionTriggered);
    action = menu.addAction(openDownloadFolderIcon, tr("Open Download Folder"));
    connect(action, &QAction::triggered, this, &TrayIcon::openDownloadFolderActionTriggered);
    action = menu.addAction(settingsIcon, tr("Settings..."));
    connect(action, &QAction::triggered, &settingsDialog, &SettingsDialog::show);
    menu.addSeparator();
    action = menu.addAction(aboutIcon, tr("About..."));
    connect(action, &QAction::triggered, &aboutDialog, &AboutDialog::show);
    action = menu.addAction(exitIcon, tr("Exit"));
    connect(action, &QAction::triggered, this, &TrayIcon::exitActionTriggered);
    setContextMenu(&menu);

    setToolTip(QApplication::applicationName());

    connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::trayIconActivated);

    server.start();
    addrPortAction->setText(tr("Port: ") + QString::number(server.port()));

    discoveryService.start(server.port());

    QTimer::singleShot(0, this, [this]() {
        showMessage(QApplication::applicationName(), QApplication::applicationName() + tr(" is launched here."));
    });
}

void TrayIcon::sendActionTriggered()
{
    SelectFilesDialog *d = new SelectFilesDialog(nullptr, discoveryService);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

void TrayIcon::openDownloadFolderActionTriggered()
{
    QString downloadPath = Settings::downloadPath();
    QDir().mkpath(downloadPath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(downloadPath));
}

void TrayIcon::exitActionTriggered()
{
    QApplication::exit();
}


void TrayIcon::trayIconActivated(ActivationReason reason)
{
    if (reason == Trigger)
        sendActionTriggered();
}

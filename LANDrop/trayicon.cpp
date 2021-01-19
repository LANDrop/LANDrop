#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QIcon>

#include "sendtodialog.h"
#include "trayicon.h"

TrayIcon::TrayIcon(QObject *parent) : QSystemTrayIcon(parent)
{
    QIcon appIcon(":/icons/app.png");
    QIcon sendIcon(":/icons/send.png");
    QIcon settingsIcon(":/icons/settings.png");
    QIcon aboutIcon(":/icons/about.png");
    QIcon exitIcon(":/icons/exit.png");
    setIcon(appIcon);

    QAction *action, *addrPortAction;
    addrPortAction = menu.addAction("");
    addrPortAction->setEnabled(false);
    menu.addSeparator();
    action = menu.addAction(sendIcon, tr("Send File(s)..."));
    connect(action, &QAction::triggered, this, &TrayIcon::sendActionTriggered);
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
}

void TrayIcon::sendActionTriggered()
{
    QStringList filenames = QFileDialog::getOpenFileNames(nullptr, tr("Select File(s) to be Sent"));
    if (filenames.empty())
        return;

    QList<QSharedPointer<QFile>> files;
    foreach (const QString &filename, filenames) {
        QSharedPointer<QFile> fp = QSharedPointer<QFile>::create(filename);
        if (!fp->open(QIODevice::ReadOnly)) {
            QMessageBox::critical(nullptr, QApplication::applicationName(),
                                  tr("Unable to open file %1. Skipping.")
                                  .arg(filename));
            continue;
        }
        if (fp->isSequential()) {
            QMessageBox::critical(nullptr, QApplication::applicationName(),
                                  tr("%1 is not a regular file. Skipping.")
                                  .arg(filename));
            continue;
        }
        files.append(fp);
    }

    if (files.empty()) {
        QMessageBox::warning(nullptr, QApplication::applicationName(), tr("No file to be sent."));
        return;
    }

    SendToDialog *d = new SendToDialog(nullptr, files, discoveryService);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

void TrayIcon::exitActionTriggered()
{
    QApplication::exit();
}


void TrayIcon::trayIconActivated(ActivationReason reason)
{
    if (reason == DoubleClick)
        sendActionTriggered();
}

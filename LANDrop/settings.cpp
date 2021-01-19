#include "settings.h"

#include <QApplication>
#include <QDir>
#include <QHostInfo>
#include <QSettings>
#include <QStandardPaths>

QString Settings::machineName()
{
    QString d = QHostInfo::localHostName();
    return QSettings().value("machineName", d).toString();
}

QString Settings::downloadPath()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    d += QDir::separator() + QApplication::applicationName();
    return QSettings().value("downloadPath", d).toString();
}

void Settings::setMachineName(const QString &machineName)
{
    QSettings().setValue("machineName", machineName);
}

void Settings::setDownloadPath(const QString &downloadPath)
{
    QSettings().setValue("downloadPath", downloadPath);
}

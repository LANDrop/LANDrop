#pragma once

#include <QString>

class Settings {
public:
    static QString machineName();
    static QString downloadPath();
    static void setMachineName(const QString &machineName);
    static void setDownloadPath(const QString &downloadPath);
};

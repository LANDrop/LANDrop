#include <QApplication>
#include <QMessageBox>
#include <QTranslator>

#include "trayicon.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("yvbbrjdr");
    a.setOrganizationDomain("yvb.moe");
    a.setApplicationName("LANDrop");
    a.setApplicationVersion("0.1.0");

    a.setQuitOnLastWindowClosed(false);

    QTranslator appTranslator;
    appTranslator.load(QLocale(), a.applicationName(), ".", ":/locales");
    a.installTranslator(&appTranslator);

    try {
        if (!QSystemTrayIcon::isSystemTrayAvailable())
            throw std::runtime_error(a.translate("Main", "Your system needs to support tray icon.")
                                     .toUtf8().toStdString());

        TrayIcon t;
        t.show();

        return a.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, QApplication::applicationName(), e.what());
        return 1;
    }
}

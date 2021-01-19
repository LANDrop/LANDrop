#pragma once

#include <QDialog>
#include <QFile>
#include <QStringListModel>
#include <QTcpSocket>
#include <QTimer>

#include "discoveryservice.h"

namespace Ui {
    class SendToDialog;
}

class SendToDialog : public QDialog {
    Q_OBJECT
public:
    explicit SendToDialog(QWidget *parent, const QList<QSharedPointer<QFile>> &files,
                          DiscoveryService &discoveryService);
    ~SendToDialog();
private:
    Ui::SendToDialog *ui;
    QList<QSharedPointer<QFile>> files;
    QStringListModel hostsStringListModel;
    struct Endpoint {
        QHostAddress addr;
        quint16 port;
    };
    QVector<Endpoint> endpoints;
    QTimer discoveryTimer;
    QTcpSocket *socket;
    QTimer socketTimeoutTimer;
private slots:
    void newHost(const QString &machineName, const QHostAddress &addr, quint16 port);
    void hostsListViewClicked(const QModelIndex &index);
    void accept();
    void socketConnected();
    void socketErrorOccurred();
    void socketTimeout();
};

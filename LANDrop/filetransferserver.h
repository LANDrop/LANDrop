#pragma once

#include <QObject>
#include <QTcpServer>

class FileTransferServer : public QObject {
    Q_OBJECT
public:
    explicit FileTransferServer(QObject *parent = nullptr);
    void start();
    quint16 port();
private:
    QTcpServer server;
private slots:
    void serverNewConnection();
};

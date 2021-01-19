#pragma once

#include <QObject>
#include <QUdpSocket>

class DiscoveryService : public QObject {
    Q_OBJECT
public:
    explicit DiscoveryService(QObject *parent = nullptr);
    void start(quint16 serverPort);
public slots:
    void refresh();
private:
    enum {
        DISCOVERY_PORT = 7638
    };
    QUdpSocket socket;
    quint16 serverPort;
    void sendInfo(const QHostAddress &addr, quint16 port);
    bool isLocalAddress(const QHostAddress &addr);
private slots:
    void socketReadyRead();
signals:
    void newHost(const QString &machineName, const QHostAddress &addr, quint16 port);
};

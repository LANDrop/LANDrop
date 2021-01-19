#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkInterface>

#include "discoveryservice.h"
#include "settings.h"

DiscoveryService::DiscoveryService(QObject *parent) : QObject(parent)
{
    connect(&socket, &QUdpSocket::readyRead, this, &DiscoveryService::socketReadyRead);
}

void DiscoveryService::start(quint16 serverPort)
{
    this->serverPort = serverPort;
    if (!socket.bind(QHostAddress::Any, DISCOVERY_PORT)) {
        QMessageBox::warning(nullptr, QApplication::applicationName(),
                             tr("Unable to bind to port %1.\nYour machine won't be discoverable.")
                             .arg(DISCOVERY_PORT));
    }
    sendInfo(QHostAddress::Broadcast, DISCOVERY_PORT);
}

void DiscoveryService::refresh()
{
    QJsonObject obj;
    obj.insert("request", true);
    socket.writeDatagram(QJsonDocument(obj).toJson(QJsonDocument::Compact), QHostAddress::Broadcast, DISCOVERY_PORT);
}

void DiscoveryService::sendInfo(const QHostAddress &addr, quint16 port)
{
    QJsonObject obj;
    obj.insert("request", false);
    obj.insert("machine_name", Settings::machineName());
    obj.insert("port", serverPort);
    socket.writeDatagram(QJsonDocument(obj).toJson(QJsonDocument::Compact), addr, port);
}

bool DiscoveryService::isLocalAddress(const QHostAddress &addr)
{
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (addr.isEqual(address))
             return true;
    }
    return false;
}

void DiscoveryService::socketReadyRead()
{
    while (socket.hasPendingDatagrams()) {
        qint64 size = socket.pendingDatagramSize();
        QByteArray data(size, 0);
        QHostAddress addr;
        quint16 port;
        socket.readDatagram(data.data(), size, &addr, &port);

        if (isLocalAddress(addr))
            continue;

        QJsonDocument json = QJsonDocument::fromJson(data);
        if (!json.isObject())
            continue;
        QJsonObject obj = json.object();
        QJsonValue request = obj.value("request");
        if (!request.isBool())
            continue;
        if (request.toBool()) {
            sendInfo(addr, port);
            continue;
        }
        QJsonValue machineName = obj.value("machine_name");
        QJsonValue remotePort = obj.value("port");
        if (!machineName.isString() || !remotePort.isDouble())
            continue;
        QString machineNameStr = machineName.toString();
        quint16 remotePortInt = remotePort.toInt();
        emit newHost(machineNameStr, addr, remotePortInt);
    }
}

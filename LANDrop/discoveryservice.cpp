/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, yvbbrjdr
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

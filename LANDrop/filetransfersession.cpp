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

#include <stdexcept>

#include "filetransfersession.h"

FileTransferSession::FileTransferSession(QObject *parent, QTcpSocket *socket) :
    QObject(parent), state(HANDSHAKE1), socket(socket), totalSize(0), transferredSize(0)
{
    socket->setParent(this);
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(socket, &QTcpSocket::readyRead, this, &FileTransferSession::socketReadyRead);
    connect(socket,
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
            &QTcpSocket::errorOccurred,
#else
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
#endif
            this, &FileTransferSession::socketErrorOccurred);
}

void FileTransferSession::start()
{
    emit printMessage(tr("Handshaking..."));
    socket->write(crypto.localPublicKey());
}

void FileTransferSession::respond(bool)
{
    throw std::runtime_error("respond not implemented");
}

void FileTransferSession::encryptAndSend(const QByteArray &data)
{
    QByteArray sendData = crypto.encrypt(data);
    quint16 size = sendData.size();
    sendData.prepend(static_cast<quint8>(size & 0xFF));
    sendData.prepend(static_cast<quint8>((size >> 8) & 0xFF));
    socket->write(sendData);
}

void FileTransferSession::handshake1Finished() {}

void FileTransferSession::socketReadyRead()
{
    readBuffer += socket->readAll();

    if (state == HANDSHAKE1) {
        if (static_cast<quint64>(readBuffer.size()) < crypto.publicKeySize()) {
            emit errorOccurred(tr("Handshake failed."));
            return;
        }
        QByteArray publicKey = readBuffer.left(crypto.publicKeySize());
        readBuffer = readBuffer.mid(crypto.publicKeySize());
        try {
            crypto.setRemotePublicKey(publicKey);
        } catch (const std::exception &e) {
            emit errorOccurred(e.what());
            return;
        }
        emit printMessage(tr("Handshaking... Code: %1").arg(crypto.sessionKeyDigest()));
        state = HANDSHAKE2;

        handshake1Finished();
    }

    while (!readBuffer.isEmpty()) {
        if (readBuffer.size() < 2)
            break;

        quint16 size = static_cast<quint16>(static_cast<quint8>(readBuffer[0])) << 8;
        size |= static_cast<quint8>(readBuffer[1]);
        if (readBuffer.size() < size + 2)
            break;

        QByteArray data = readBuffer.mid(2, size);
        readBuffer = readBuffer.mid(size + 2);

        try {
            data = crypto.decrypt(data);
        } catch (const std::exception &e) {
            emit errorOccurred(e.what());
            return;
        }

        processReceivedData(data);
    }
}

void FileTransferSession::socketErrorOccurred()
{
    if (state != FINISHED)
        emit errorOccurred(socket->errorString());
}

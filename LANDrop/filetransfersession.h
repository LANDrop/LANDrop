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

#pragma once

#include <QObject>
#include <QTcpSocket>

#include "crypto.h"

class FileTransferSession : public QObject {
    Q_OBJECT
public:
    struct FileMetadata {
        QString filename;
        quint64 size;
    };
    explicit FileTransferSession(QObject *parent, QTcpSocket *socket);
    void start();
    virtual void respond(bool accepted);
protected:
    enum State {
        HANDSHAKE1,
        HANDSHAKE2,
        TRANSFERRING,
        FINISHED
    } state;
    QTcpSocket *socket;
    Crypto crypto;
    QByteArray readBuffer;
    QList<FileMetadata> transferQ;
    quint64 totalSize;
    quint64 transferredSize;
    void encryptAndSend(const QByteArray &data);
    virtual void handshake1Finished();
    virtual void processReceivedData(const QByteArray &data) = 0;
private slots:
    void socketReadyRead();
    void socketErrorOccurred();
signals:
    void printMessage(const QString &msg);
    void updateProgress(double progress);
    void errorOccurred(const QString &msg);
    void fileMetadataReady(const QList<FileTransferSession::FileMetadata> &metadata, quint64 totalSize,
                           const QString &deviceName, const QString &sessionKeyDigest);
    void ended();
};

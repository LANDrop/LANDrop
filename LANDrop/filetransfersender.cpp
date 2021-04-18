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

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include "filetransfersender.h"
#include "settings.h"

FileTransferSender::FileTransferSender(QObject *parent, QTcpSocket *socket, const QList<QSharedPointer<QFile>> &files) :
    FileTransferSession(parent, socket), files(files)
{
    connect(socket, &QTcpSocket::bytesWritten, this, &FileTransferSender::socketBytesWritten);

    foreach (QSharedPointer<QFile> file, files) {
        QString filename = QFileInfo(*file).fileName();
        quint64 size = static_cast<quint64>(file->size());
        totalSize += size;
        transferQ.append({filename, size});
    }
}

void FileTransferSender::handshake1Finished()
{
    QJsonArray jsonFiles;
    foreach (FileMetadata metadata, transferQ) {
        QJsonObject jsonFile;
        jsonFile.insert("filename", metadata.filename);
        jsonFile.insert("size", static_cast<qint64>(metadata.size));
        jsonFiles.append(jsonFile);
    }

    QJsonObject obj;
    obj.insert("device_name", Settings::deviceName());
    obj.insert("device_type", QSysInfo::productType());
    obj.insert("files", jsonFiles);
    encryptAndSend(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void FileTransferSender::processReceivedData(const QByteArray &data)
{
    if (state == HANDSHAKE2) {
        QJsonDocument json = QJsonDocument::fromJson(data);
        if (!json.isObject()) {
            emit errorOccurred(tr("Handshake failed."));
            return;
        }

        QJsonObject obj = json.object();
            QJsonValue response = obj.value("response");
            if (!response.isDouble()) {
                emit errorOccurred(tr("Handshake failed."));
                return;
            }

            if (response.toInt() == 0) {
                emit errorOccurred(tr("The receiving device rejected your file(s)."));
                return;
            }
            state = TRANSFERRING;
            socketBytesWritten();
    }
}

void FileTransferSender::socketBytesWritten()
{
    if (state != TRANSFERRING || socket->bytesToWrite() > 0)
        return;

    while (!transferQ.empty()) {
        FileMetadata &curFile = transferQ.front();
        if (curFile.size == 0) {
            transferQ.pop_front();
            files.pop_front();
        } else {
            emit printMessage(tr("Sending file %1...").arg(curFile.filename));
            break;
        }
    }
    if (transferQ.empty()) {
        state = FINISHED;
        emit printMessage(tr("Done!"));
        socket->disconnectFromHost();
        QTimer::singleShot(5000, this, &FileTransferSession::ended);
        return;
    }
    QSharedPointer<QFile> &curFile = files.front();
    FileMetadata &curMetadata = transferQ.front();
    QByteArray data = curFile->read(TRANSFER_QUANTA);
    encryptAndSend(data);
    curMetadata.size -= data.size();
    transferredSize += data.size();
    emit updateProgress(static_cast<double>(transferredSize) / totalSize);
}

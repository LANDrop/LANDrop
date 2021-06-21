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

#include <QDesktopServices>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

#include "filetransferreceiver.h"
#include "settings.h"

FileTransferReceiver::FileTransferReceiver(QObject *parent, QTcpSocket *socket) :
    FileTransferSession(parent, socket), writingFile(nullptr), downloadPath(Settings::downloadPath()) {}

void FileTransferReceiver::respond(bool accepted)
{
    QJsonObject obj;
    obj.insert("response", static_cast<int>(accepted));
    encryptAndSend(QJsonDocument(obj).toJson(QJsonDocument::Compact));

    if (accepted) {
        if (!QDir().mkpath(downloadPath)) {
            emit errorOccurred(tr("Cannot create download path: ") + downloadPath);
            return;
        }
        if (!QFileInfo(downloadPath).isWritable()) {
            emit errorOccurred(tr("Download path is not writable: ") + downloadPath);
            return;
        }
        state = TRANSFERRING;
        createNextFile();
    } else {
        connect(socket, &QTcpSocket::bytesWritten, this, &FileTransferReceiver::ended);
    }
}

void FileTransferReceiver::processReceivedData(const QByteArray &data)
{
    if (state == HANDSHAKE2) {
        QJsonDocument json = QJsonDocument::fromJson(data);
        if (!json.isObject()) {
            emit ended();
            return;
        }

        QJsonObject obj = json.object();
        QJsonValue deviceName = obj.value("device_name");
        if (!deviceName.isString()) {
            emit ended();
            return;
        }

        QJsonValue filesJson = obj.value("files");
        if (!filesJson.isArray()) {
            emit ended();
            return;
        }

        QJsonArray filesJsonArray = filesJson.toArray();
        if (filesJsonArray.empty()) {
            emit ended();
            return;
        }

        foreach (const QJsonValue &v, filesJsonArray) {
            if (!v.isObject()) {
                emit ended();
                return;
            }
            QJsonObject o = v.toObject();

            QJsonValue filename = o.value("filename");
            if (!filename.isString()) {
                emit ended();
                return;
            }

            QJsonValue size = o.value("size");
            if (!size.isDouble()) {
                emit ended();
                return;
            }

            quint64 sizeInt = static_cast<quint64>(size.toDouble());
            totalSize += sizeInt;
            transferQ.append({filename.toString(), sizeInt});
        }

        emit fileMetadataReady(transferQ, totalSize, deviceName.toString(),
                               crypto.sessionKeyDigest());
    } else if (state == TRANSFERRING) {
        transferredSize += data.size();
        emit updateProgress(static_cast<double>(transferredSize) / totalSize);
        QByteArray tmpData = data;
        while (tmpData.size() > 0) {
            FileMetadata &curFile = transferQ.first();
            quint64 writeSize = qMin(curFile.size, static_cast<quint64>(tmpData.size()));
            qint64 written = writingFile->write(tmpData.left(writeSize));
            curFile.size -= written;
            tmpData = tmpData.mid(written);
            if (curFile.size == 0) {
                transferQ.pop_front();
                createNextFile();
            }
        }
    }
}

void FileTransferReceiver::createNextFile()
{
    while (!transferQ.empty()) {
        FileMetadata &curFile = transferQ.first();
        QString filename = downloadPath + QDir::separator() + curFile.filename;
        if (writingFile) {
            writingFile->deleteLater();
            writingFile = nullptr;
        }
        writingFile = new QFile(filename, this);
        if (!writingFile->open(QIODevice::WriteOnly)) {
            emit errorOccurred(tr("Unable to open file %1.").arg(filename));
            return;
        }
        if (curFile.size > 0) {
            emit printMessage(tr("Receiving file %1...").arg(curFile.filename));
            break;
        }
        transferQ.pop_front();
    }
    if (transferQ.empty()) {
        if (writingFile) {
            writingFile->deleteLater();
            writingFile = nullptr;
        }
        state = FINISHED;
        QDesktopServices::openUrl(QUrl::fromLocalFile(downloadPath));
        emit printMessage(tr("Done!"));
        socket->disconnectFromHost();
        QTimer::singleShot(5000, this, &FileTransferSession::ended);
    }
}

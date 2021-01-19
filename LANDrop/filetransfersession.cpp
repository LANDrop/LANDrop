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

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

#include "filetransfersession.h"
#include "settings.h"

FileTransferSession::FileTransferSession(QObject *parent, TransferDirection dir, QTcpSocket *socket,
                                         const QList<QSharedPointer<QFile>> &files) :
    QObject(parent), state(HANDSHAKE1), dir(dir), socket(socket), files(files),
    totalSize(0), transferredSize(0), writingFile(nullptr), downloadPath(Settings::downloadPath())
{
    socket->setParent(this);
    connect(socket, &QTcpSocket::readyRead, this, &FileTransferSession::socketReadyRead);
    connect(socket, &QTcpSocket::bytesWritten, this, &FileTransferSession::socketBytesWritten);
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

void FileTransferSession::respond(bool accepted)
{
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
        state = FINISHED;
    }

    QJsonObject obj;
    obj.insert("response", static_cast<int>(accepted));
    encryptAndSend(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void FileTransferSession::encryptAndSend(const QByteArray &data)
{
    QByteArray sendData = crypto.encrypt(data);
    quint16 size = sendData.size();
    sendData.prepend(static_cast<quint8>(size & 0xFF));
    sendData.prepend(static_cast<quint8>((size >> 8) & 0xFF));
    socket->write(sendData);
}

void FileTransferSession::processReceivedData(const QByteArray &data)
{
    if (state == HANDSHAKE2) {
        QJsonDocument json = QJsonDocument::fromJson(data);
        if (!json.isObject()) {
            if (dir == SENDING)
                emit errorOccurred(tr("Handshake failed."));
            else
                emit ended();
            return;
        }

        QJsonObject obj = json.object();
        if (dir == SENDING) {
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
        } else {
            QJsonValue machineName = obj.value("machine_name");
            if (!machineName.isString()) {
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

            emit fileMetadataReady(transferQ, totalSize, machineName.toString(),
                                   crypto.sessionKeyDigest());
        }
        return;
    }

    if (state != TRANSFERRING || dir != RECEIVING)
        return;

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

void FileTransferSession::createNextFile()
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
        state = FINISHED;
        QDesktopServices::openUrl(QUrl::fromLocalFile(downloadPath));
        emit printMessage(tr("Done!"));
        QTimer::singleShot(5000, this, &FileTransferSession::ended);
    }
}

void FileTransferSession::socketReadyRead()
{
    readBuffer += socket->readAll();

    if (state == HANDSHAKE1) {
        if (static_cast<quint64>(readBuffer.size()) < crypto.publicKeySize()) {
            if (dir == SENDING)
                emit errorOccurred(tr("Handshake failed."));
            else
                emit ended();
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

        if (dir == SENDING) {
            QJsonArray jsonFiles;
            foreach (QSharedPointer<QFile> file, files) {
                QString filename = QFileInfo(*file).fileName();
                quint64 size = static_cast<quint64>(file->size());
                totalSize += size;
                QJsonObject jsonFile;
                jsonFile.insert("filename", filename);
                jsonFile.insert("size", static_cast<qint64>(size));
                jsonFiles.append(jsonFile);

                transferQ.append({filename, size});
            }

            QJsonObject obj;
            obj.insert("machine_name", Settings::machineName());
            obj.insert("files", jsonFiles);
            encryptAndSend(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        }
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

void FileTransferSession::socketBytesWritten()
{
    if (dir != SENDING || state != TRANSFERRING || socket->bytesToWrite() > 0)
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

void FileTransferSession::socketErrorOccurred()
{
    if ((dir == RECEIVING && state != TRANSFERRING) || state == FINISHED) {
        emit ended();
        return;
    }
    emit errorOccurred(socket->errorString());
}

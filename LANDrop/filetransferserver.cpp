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

#include "filetransferdialog.h"
#include "filetransferreceiver.h"
#include "filetransferserver.h"
#include "settings.h"

FileTransferServer::FileTransferServer(QObject *parent) : QObject(parent) {}

void FileTransferServer::start()
{
    quint16 port = Settings::serverPort();
    if (!server.listen(QHostAddress::Any, port))
        throw std::runtime_error(tr("Unable to listen on port %1.").arg(port).toUtf8().toStdString());

    connect(&server, &QTcpServer::newConnection, this, &FileTransferServer::serverNewConnection);
}

quint16 FileTransferServer::port()
{
    return server.serverPort();
}

void FileTransferServer::serverNewConnection()
{
    while (server.hasPendingConnections()) {
        FileTransferReceiver *receiver = new FileTransferReceiver(nullptr, server.nextPendingConnection());
        FileTransferDialog *d = new FileTransferDialog(nullptr, receiver);
        d->setAttribute(Qt::WA_DeleteOnClose);
    }
}

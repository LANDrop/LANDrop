#include "filetransferdialog.h"
#include "filetransferserver.h"

FileTransferServer::FileTransferServer(QObject *parent) : QObject(parent) {}

void FileTransferServer::start()
{
    if (!server.listen())
        throw std::runtime_error(tr("Unable to listen on a port.").toUtf8().toStdString());

    connect(&server, &QTcpServer::newConnection, this, &FileTransferServer::serverNewConnection);
}

quint16 FileTransferServer::port()
{
    return server.serverPort();
}

void FileTransferServer::serverNewConnection()
{
    while (server.hasPendingConnections()) {
        FileTransferDialog *d = new FileTransferDialog(nullptr, FileTransferSession::RECEIVING,
                                                       server.nextPendingConnection());
        d->setAttribute(Qt::WA_DeleteOnClose);
    }
}

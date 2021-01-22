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

#include <QMessageBox>
#include <QPushButton>

#include "filetransferdialog.h"
#include "sendtodialog.h"
#include "ui_sendtodialog.h"

SendToDialog::SendToDialog(QWidget *parent, const QList<QSharedPointer<QFile>> &files,
                           DiscoveryService &discoveryService) :
    QDialog(parent), ui(new Ui::SendToDialog), files(files)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    ui->hostsListView->setModel(&hostsStringListModel);
    ui->hostsListView->setEditTriggers(QListView::NoEditTriggers);
    connect(ui->hostsListView, &QListView::clicked, this, &SendToDialog::hostsListViewClicked);
    connect(ui->hostsListView, &QListView::doubleClicked, ui->buttonBox, &QDialogButtonBox::accepted);

    connect(&discoveryService, &DiscoveryService::newHost, this, &SendToDialog::newHost);
    connect(&discoveryTimer, &QTimer::timeout, &discoveryService, &DiscoveryService::refresh);
    discoveryTimer.start(1000);
    discoveryService.refresh();

    connect(&socketTimeoutTimer, &QTimer::timeout, this, &SendToDialog::socketTimeout);
    socketTimeoutTimer.setSingleShot(true);
}

SendToDialog::~SendToDialog()
{
    delete ui;
}

void SendToDialog::newHost(const QString &machineName, const QHostAddress &addr, quint16 port)
{
    QStringList l = hostsStringListModel.stringList();
    for (int i = 0; i < endpoints.size(); ++i) {
        if (endpoints[i].addr.isEqual(addr) && endpoints[i].port == port) {
            if (l.at(i) != machineName) {
                l.replace(i, machineName);
                hostsStringListModel.setStringList(l);
            }
            return;
        }
    }
    endpoints.append({addr, port});
    l.append(machineName);
    hostsStringListModel.setStringList(l);
}

void SendToDialog::hostsListViewClicked(const QModelIndex &index)
{
    Endpoint endpoint = endpoints[index.row()];
    bool isV4;
    quint32 ipv4 = endpoint.addr.toIPv4Address(&isV4);
    QString addr = isV4 ? QHostAddress(ipv4).toString() : endpoint.addr.toString();
    ui->addrLineEdit->setText(addr);
    ui->portLineEdit->setText(QString::number(endpoint.port));
}

void SendToDialog::accept()
{
    QString addr = ui->addrLineEdit->text();
    bool ok;
    quint16 port = ui->portLineEdit->text().toUShort(&ok);
    if (!ok) {
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Invalid port. Please enter a number between 1 and 65535."));
        return;
    }

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &SendToDialog::socketConnected);
    connect(socket,
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
            &QTcpSocket::errorOccurred,
#else
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
#endif
            this, &SendToDialog::socketErrorOccurred);
    socket->connectToHost(addr, port);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    setCursor(QCursor(Qt::WaitCursor));
    socketTimeoutTimer.start(5000);
}

void SendToDialog::socketConnected()
{
    socketTimeoutTimer.stop();
    FileTransferDialog *d = new FileTransferDialog(nullptr, FileTransferSession::SENDING, socket, files);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
    done(Accepted);
}

void SendToDialog::socketErrorOccurred()
{
    socketTimeoutTimer.stop();
    socket->disconnectFromHost();
    socket->close();
    socket->deleteLater();
    QMessageBox::critical(this, QApplication::applicationName(), socket->errorString());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    setCursor(QCursor(Qt::ArrowCursor));
}

void SendToDialog::socketTimeout()
{
    socket->disconnectFromHost();
    socket->close();
    socket->deleteLater();
    QMessageBox::critical(this, QApplication::applicationName(), tr("Connection timed out"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    setCursor(QCursor(Qt::ArrowCursor));
}

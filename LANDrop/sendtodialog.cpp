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

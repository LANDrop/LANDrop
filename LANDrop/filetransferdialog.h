#pragma once

#include <QDialog>
#include <QFile>
#include <QTcpSocket>

#include "filetransfersession.h"

namespace Ui {
    class FileTransferDialog;
}

class FileTransferDialog : public QDialog {
    Q_OBJECT
public:
    explicit FileTransferDialog(QWidget *parent, FileTransferSession::TransferDirection dir, QTcpSocket *socket,
                                const QList<QSharedPointer<QFile>> &files = QList<QSharedPointer<QFile>>());
    ~FileTransferDialog();
private:
    Ui::FileTransferDialog *ui;
    FileTransferSession session;
private slots:
    void sessionUpdateProgress(double progress);
    void sessionErrorOccurred(const QString &msg);
    void sessionFileMetadataReady(const QList<FileTransferSession::FileMetadata> &metadata, quint64 totalSize,
                                  const QString &machineName, const QString &sessionKeyDigest);
};

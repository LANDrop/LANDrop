#pragma once

#include <QObject>
#include <QFile>
#include <QTcpSocket>

#include "crypto.h"

class FileTransferSession : public QObject {
    Q_OBJECT
public:
    enum TransferDirection {
        SENDING,
        RECEIVING
    };
    struct FileMetadata {
        QString filename;
        quint64 size;
    };
    explicit FileTransferSession(QObject *parent, TransferDirection dir, QTcpSocket *socket,
                                 const QList<QSharedPointer<QFile>> &files);
    void start();
    void respond(bool accepted);
private:
    enum State {
        HANDSHAKE1,
        HANDSHAKE2,
        TRANSFERRING,
        FINISHED
    } state;
    enum {
        TRANSFER_QUANTA = 64000
    };
    TransferDirection dir;
    QTcpSocket *socket;
    QList<QSharedPointer<QFile>> files;
    Crypto crypto;
    QByteArray readBuffer;
    QList<FileMetadata> transferQ;
    quint64 totalSize;
    quint64 transferredSize;
    QFile *writingFile;
    QString downloadPath;
    void encryptAndSend(const QByteArray &data);
    void processReceivedData(const QByteArray &data);
    void createNextFile();
private slots:
    void socketReadyRead();
    void socketBytesWritten();
    void socketErrorOccurred();
signals:
    void printMessage(const QString &msg);
    void updateProgress(double progress);
    void errorOccurred(const QString &msg);
    void fileMetadataReady(const QList<FileMetadata> &metadata, quint64 totalSize,
                           const QString &machineName, const QString &sessionKeyDigest);
    void ended();
};

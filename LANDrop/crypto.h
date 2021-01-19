#pragma once

#include <QApplication>
#include <QByteArray>
#include <QString>

class Crypto {
    Q_DECLARE_TR_FUNCTIONS(Crypto)
public:
    Crypto();
    quint64 publicKeySize();
    QByteArray localPublicKey();
    void setRemotePublicKey(const QByteArray &remotePublicKey);
    QString sessionKeyDigest();
    QByteArray encrypt(const QByteArray &data);
    QByteArray decrypt(const QByteArray &data);
private:
    static bool inited;
    QByteArray publicKey;
    QByteArray secretKey;
    QByteArray sessionKey;
    static void init();
};

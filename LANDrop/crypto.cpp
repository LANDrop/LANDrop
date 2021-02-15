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

#include <sodium.h>

#include "crypto.h"

bool Crypto::inited = false;

Crypto::Crypto() :
    publicKey(crypto_scalarmult_BYTES, 0),
    secretKey(crypto_scalarmult_SCALARBYTES, 0),
    sessionKey(crypto_aead_chacha20poly1305_IETF_KEYBYTES, 0)
{
    init();
    randombytes_buf(secretKey.data(), secretKey.size());
    crypto_scalarmult_base(reinterpret_cast<unsigned char *>(publicKey.data()),
                           reinterpret_cast<const unsigned char *>(secretKey.data()));
}

quint64 Crypto::publicKeySize()
{
    return crypto_aead_chacha20poly1305_IETF_KEYBYTES;
}

QByteArray Crypto::localPublicKey()
{
    return publicKey;
}

void Crypto::setRemotePublicKey(const QByteArray &remotePublicKey)
{
    if (crypto_scalarmult(reinterpret_cast<unsigned char *>(sessionKey.data()),
                          reinterpret_cast<const unsigned char *>(secretKey.data()),
                          reinterpret_cast<const unsigned char *>(remotePublicKey.data())) != 0)
        throw std::runtime_error(tr("Unable to calculate session key.").toUtf8().toStdString());
}

QString Crypto::sessionKeyDigest()
{
    QByteArray h(crypto_generichash_BYTES_MIN, 0);
    crypto_generichash(reinterpret_cast<unsigned char *>(h.data()), h.size(),
                       reinterpret_cast<const unsigned char *>(sessionKey.data()), sessionKey.size(),
                       nullptr, 0);
    quint64 hash = 0;
    for (int i = 0; i < 8; ++i)
        hash |= static_cast<quint64>(static_cast<quint8>(h[i])) << (i * 8);
    return QString("%1").arg(hash % 1000000, 6, 10, QLatin1Char('0'));
}

QByteArray Crypto::encrypt(const QByteArray &data)
{
    QByteArray cipherText(data.size() + crypto_aead_chacha20poly1305_IETF_ABYTES, 0);
    quint64 cipherTextLen;
    QByteArray nonce(crypto_aead_chacha20poly1305_IETF_NPUBBYTES, 0);
    randombytes_buf(nonce.data(), nonce.size());
    crypto_aead_chacha20poly1305_ietf_encrypt(reinterpret_cast<unsigned char *>(cipherText.data()), &cipherTextLen,
                                              reinterpret_cast<const unsigned char *>(data.data()), data.size(),
                                              nullptr, 0, nullptr,
                                              reinterpret_cast<const unsigned char *>(nonce.data()),
                                              reinterpret_cast<const unsigned char *>(sessionKey.data()));
    return nonce + cipherText.left(cipherTextLen);
}

QByteArray Crypto::decrypt(const QByteArray &data)
{
    if (static_cast<quint64>(data.size()) < crypto_aead_chacha20poly1305_IETF_NPUBBYTES)
        throw std::runtime_error(tr("Cipher text too short.").toUtf8().toStdString());
    QByteArray plainText(data.size() - crypto_aead_chacha20poly1305_IETF_ABYTES, 0);
    quint64 plainTextLen;
    QByteArray nonce = data.left(crypto_aead_chacha20poly1305_IETF_NPUBBYTES);
    QByteArray cipherText = data.mid(crypto_aead_chacha20poly1305_IETF_NPUBBYTES);
    if (crypto_aead_chacha20poly1305_ietf_decrypt(reinterpret_cast<unsigned char *>(plainText.data()), &plainTextLen,
                                                  nullptr,
                                                  reinterpret_cast<const unsigned char *>(cipherText.data()), cipherText.size(),
                                                  nullptr, 0,
                                                  reinterpret_cast<const unsigned char *>(nonce.data()),
                                                  reinterpret_cast<const unsigned char *>(sessionKey.data())) != 0)
        throw std::runtime_error(tr("Decryption failed.").toUtf8().toStdString());
    return plainText.left(plainTextLen);
}

void Crypto::init()
{
    if (inited)
        return;
    if (sodium_init() == -1)
        throw std::runtime_error(tr("Unable to initialize libsodium.").toUtf8().toStdString());
    inited = true;
}

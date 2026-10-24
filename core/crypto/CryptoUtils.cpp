#include "CryptoUtils.hpp"

#include <QCryptographicHash>
#include <sodium.h>
#include <mutex>

namespace peerchat::crypto {

static std::once_flag s_sodium_init_flag;
static bool s_sodium_initialized = false;

bool CryptoUtils::initialize() {
    std::call_once(s_sodium_init_flag, []() {
        if (sodium_init() >= 0) {
            s_sodium_initialized = true;
        }
    });
    return s_sodium_initialized;
}

QByteArray CryptoUtils::sha256(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QString CryptoUtils::sha256Hex(const QByteArray& data) {
    return QString::fromUtf8(sha256(data).toHex());
}

QByteArray CryptoUtils::randomBytes(size_t length) {
    initialize();
    QByteArray buffer(static_cast<qsizetype>(length), 0);
    randombytes_buf(buffer.data(), length);
    return buffer;
}

CryptoUtils::KeyPair CryptoUtils::generateEd25519KeyPair() {
    initialize();
    KeyPair kp;
    kp.publicKey.resize(crypto_sign_ed25519_PUBLICKEYBYTES);
    kp.secretKey.resize(crypto_sign_ed25519_SECRETKEYBYTES);

    crypto_sign_ed25519_keypair(
        reinterpret_cast<unsigned char*>(kp.publicKey.data()),
        reinterpret_cast<unsigned char*>(kp.secretKey.data())
    );
    return kp;
}

QByteArray CryptoUtils::sign(const QByteArray& message, const QByteArray& secretKey) {
    initialize();
    if (secretKey.size() != crypto_sign_ed25519_SECRETKEYBYTES) {
        return {};
    }

    QByteArray signature(crypto_sign_ed25519_BYTES, 0);
    unsigned long long sig_len = 0;
    crypto_sign_ed25519_detached(
        reinterpret_cast<unsigned char*>(signature.data()),
        &sig_len,
        reinterpret_cast<const unsigned char*>(message.constData()),
        static_cast<unsigned long long>(message.size()),
        reinterpret_cast<const unsigned char*>(secretKey.constData())
    );
    signature.resize(static_cast<qsizetype>(sig_len));
    return signature;
}

bool CryptoUtils::verify(const QByteArray& message, const QByteArray& signature, const QByteArray& publicKey) {
    initialize();
    if (publicKey.size() != crypto_sign_ed25519_PUBLICKEYBYTES || signature.size() != crypto_sign_ed25519_BYTES) {
        return false;
    }

    int ret = crypto_sign_ed25519_verify_detached(
        reinterpret_cast<const unsigned char*>(signature.constData()),
        reinterpret_cast<const unsigned char*>(message.constData()),
        static_cast<unsigned long long>(message.size()),
        reinterpret_cast<const unsigned char*>(publicKey.constData())
    );
    return (ret == 0);
}

std::optional<QByteArray> CryptoUtils::ed25519PkToCurve25519(const QByteArray& ed25519Pk) {
    initialize();
    if (ed25519Pk.size() != crypto_sign_ed25519_PUBLICKEYBYTES) {
        return std::nullopt;
    }

    QByteArray curvePk(crypto_scalarmult_curve25519_BYTES, 0);
    if (crypto_sign_ed25519_pk_to_curve25519(
            reinterpret_cast<unsigned char*>(curvePk.data()),
            reinterpret_cast<const unsigned char*>(ed25519Pk.constData())) != 0) {
        return std::nullopt;
    }
    return curvePk;
}

std::optional<QByteArray> CryptoUtils::ed25519SkToCurve25519(const QByteArray& ed25519Sk) {
    initialize();
    if (ed25519Sk.size() != crypto_sign_ed25519_SECRETKEYBYTES) {
        return std::nullopt;
    }

    QByteArray curveSk(crypto_scalarmult_curve25519_BYTES, 0);
    if (crypto_sign_ed25519_sk_to_curve25519(
            reinterpret_cast<unsigned char*>(curveSk.data()),
            reinterpret_cast<const unsigned char*>(ed25519Sk.constData())) != 0) {
        return std::nullopt;
    }
    return curveSk;
}

QByteArray CryptoUtils::encrypt(const QByteArray& plaintext, const QByteArray& key, const QByteArray& nonce) {
    initialize();
    if (key.size() != crypto_aead_xchacha20poly1305_ietf_KEYBYTES ||
        nonce.size() != crypto_aead_xchacha20poly1305_ietf_NPUBBYTES) {
        return {};
    }

    QByteArray ciphertext(plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES, 0);
    unsigned long long clen = 0;

    crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char*>(ciphertext.data()),
        &clen,
        reinterpret_cast<const unsigned char*>(plaintext.constData()),
        static_cast<unsigned long long>(plaintext.size()),
        nullptr, 0, // no additional authenticated data
        nullptr,
        reinterpret_cast<const unsigned char*>(nonce.constData()),
        reinterpret_cast<const unsigned char*>(key.constData())
    );
    ciphertext.resize(static_cast<qsizetype>(clen));
    return ciphertext;
}

std::optional<QByteArray> CryptoUtils::decrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& nonce) {
    initialize();
    if (key.size() != crypto_aead_xchacha20poly1305_ietf_KEYBYTES ||
        nonce.size() != crypto_aead_xchacha20poly1305_ietf_NPUBBYTES ||
        ciphertext.size() < crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        return std::nullopt;
    }

    QByteArray decrypted(ciphertext.size() - crypto_aead_xchacha20poly1305_ietf_ABYTES, 0);
    unsigned long long mlen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            reinterpret_cast<unsigned char*>(decrypted.data()),
            &mlen,
            nullptr,
            reinterpret_cast<const unsigned char*>(ciphertext.constData()),
            static_cast<unsigned long long>(ciphertext.size()),
            nullptr, 0,
            reinterpret_cast<const unsigned char*>(nonce.constData()),
            reinterpret_cast<const unsigned char*>(key.constData())) != 0) {
        return std::nullopt;
    }
    decrypted.resize(static_cast<qsizetype>(mlen));
    return decrypted;
}

QString CryptoUtils::toHex(const QByteArray& data) {
    return QString::fromUtf8(data.toHex());
}

QByteArray CryptoUtils::fromHex(const QString& hex) {
    return QByteArray::fromHex(hex.toUtf8());
}

QString CryptoUtils::toBase64(const QByteArray& data) {
    return QString::fromUtf8(data.toBase64());
}

QByteArray CryptoUtils::fromBase64(const QString& b64) {
    return QByteArray::fromBase64(b64.toUtf8());
}

}  // namespace peerchat::crypto

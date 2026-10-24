#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>
#include <optional>
#include <string>

namespace peerchat::crypto {

class CryptoUtils {
public:
    // Initialize libsodium (thread-safe, idempotent)
    static bool initialize();

    // Cryptographic hashing (SHA-256)
    static QByteArray sha256(const QByteArray& data);
    static QString sha256Hex(const QByteArray& data);

    // Random byte generation
    static QByteArray randomBytes(size_t length);

    // Ed25519 Signing Keypair generation
    struct KeyPair {
        QByteArray publicKey;  // 32 bytes
        QByteArray secretKey;  // 64 bytes
    };
    static KeyPair generateEd25519KeyPair();

    // Sign and verify with Ed25519
    static QByteArray sign(const QByteArray& message, const QByteArray& secretKey);
    static bool verify(const QByteArray& message, const QByteArray& signature, const QByteArray& publicKey);

    // Convert Ed25519 public key to X25519 for diffie-hellman key exchange
    static std::optional<QByteArray> ed25519PkToCurve25519(const QByteArray& ed25519Pk);
    static std::optional<QByteArray> ed25519SkToCurve25519(const QByteArray& ed25519Sk);

    // Authenticated symmetric encryption (ChaCha20-Poly1305)
    // Nonce must be 24 bytes (XChaCha20) or 12 bytes
    static QByteArray encrypt(const QByteArray& plaintext, const QByteArray& key, const QByteArray& nonce);
    static std::optional<QByteArray> decrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& nonce);

    // Encodings
    static QString toHex(const QByteArray& data);
    static QByteArray fromHex(const QString& hex);

    static QString toBase64(const QByteArray& data);
    static QByteArray fromBase64(const QString& b64);
};

}  // namespace peerchat::crypto

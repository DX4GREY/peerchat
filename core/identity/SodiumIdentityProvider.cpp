#include "SodiumIdentityProvider.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <sodium.h>

namespace peerchat::identity {

QByteArray IIdentityProvider::extractPublicKey(const QString& peerId) {
    // Expected format: "peer:ed25519:<64_hex_chars>"
    if (!peerId.startsWith("peer:ed25519:")) {
        return {};
    }
    QString hex = peerId.mid(13);
    if (hex.length() != 64) {
        return {};
    }
    return QByteArray::fromHex(hex.toUtf8());
}

QString IIdentityProvider::formatPeerId(const QByteArray& publicKey) {
    if (publicKey.size() != crypto_sign_ed25519_PUBLICKEYBYTES) {
        return {};
    }
    return QStringLiteral("peer:ed25519:") + QString::fromUtf8(publicKey.toHex());
}

SodiumIdentityProvider::SodiumIdentityProvider(QObject* parent) : IIdentityProvider(parent) {
    crypto::CryptoUtils::initialize();
}

SodiumIdentityProvider::~SodiumIdentityProvider() {
    // Securely wipe secret key in memory
    if (!m_keyPair.secretKey.isEmpty()) {
        sodium_memzero(m_keyPair.secretKey.data(), static_cast<size_t>(m_keyPair.secretKey.size()));
    }
}

bool SodiumIdentityProvider::loadOrCreate(const QString& keyStoragePath,
                                          const QString& displayName) {
    m_keyPath = keyStoragePath;
    QFileInfo fi(m_keyPath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (QFile::exists(m_keyPath)) {
        QFile file(m_keyPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray docData = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(docData);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QByteArray pubHex = obj.value("public_key").toString().toUtf8();
                QByteArray secHex = obj.value("secret_key").toString().toUtf8();
                QString savedName = obj.value("display_name").toString();

                m_keyPair.publicKey = QByteArray::fromHex(pubHex);
                m_keyPair.secretKey = QByteArray::fromHex(secHex);
                if (!displayName.trimmed().isEmpty()) {
                    m_displayName = displayName.trimmed();
                } else if (!savedName.trimmed().isEmpty()) {
                    m_displayName = savedName.trimmed();
                }

                if (m_keyPair.publicKey.size() == crypto_sign_ed25519_PUBLICKEYBYTES &&
                    m_keyPair.secretKey.size() == crypto_sign_ed25519_SECRETKEYBYTES) {
                    m_loaded = true;
                    emit identityChanged();
                    return true;
                }
            }
        }
    }

    // Generate new keypair
    m_keyPair = crypto::CryptoUtils::generateEd25519KeyPair();
    if (!displayName.trimmed().isEmpty()) {
        m_displayName = displayName.trimmed();
    } else {
        m_displayName =
            QStringLiteral("Peer_") + QString::fromUtf8(m_keyPair.publicKey.left(4).toHex());
    }

    QJsonObject obj;
    obj["public_key"] = QString::fromUtf8(m_keyPair.publicKey.toHex());
    obj["secret_key"] = QString::fromUtf8(m_keyPair.secretKey.toHex());
    obj["display_name"] = m_displayName;

    QJsonDocument doc(obj);
    QFile file(m_keyPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        m_loaded = true;
        emit identityChanged();
        return true;
    }

    return false;
}

QString SodiumIdentityProvider::peerId() const {
    return formatPeerId(m_keyPair.publicKey);
}

QString SodiumIdentityProvider::displayName() const {
    return m_displayName;
}

void SodiumIdentityProvider::setDisplayName(const QString& name) {
    if (m_displayName != name && !name.trimmed().isEmpty()) {
        m_displayName = name.trimmed();
        if (m_loaded && !m_keyPath.isEmpty()) {
            // Update saved file
            QJsonObject obj;
            obj["public_key"] = QString::fromUtf8(m_keyPair.publicKey.toHex());
            obj["secret_key"] = QString::fromUtf8(m_keyPair.secretKey.toHex());
            obj["display_name"] = m_displayName;
            QJsonDocument doc(obj);
            QFile file(m_keyPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
            }
        }
        emit identityChanged();
    }
}

QByteArray SodiumIdentityProvider::publicKey() const {
    return m_keyPair.publicKey;
}

QByteArray SodiumIdentityProvider::secretKey() const {
    return m_keyPair.secretKey;
}

QByteArray SodiumIdentityProvider::sign(const QByteArray& data) const {
    return crypto::CryptoUtils::sign(data, m_keyPair.secretKey);
}

bool SodiumIdentityProvider::verifyPeerSignature(const QString& peerId, const QByteArray& data,
                                                 const QByteArray& signature) const {
    QByteArray pubKey = extractPublicKey(peerId);
    if (pubKey.isEmpty()) {
        return false;
    }
    return crypto::CryptoUtils::verify(data, signature, pubKey);
}

}  // namespace peerchat::identity

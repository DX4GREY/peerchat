#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace peerchat::identity {

class IIdentityProvider : public QObject {
    Q_OBJECT
public:
    explicit IIdentityProvider(QObject* parent = nullptr) : QObject(parent) {}
    ~IIdentityProvider() override = default;

    // Load or generate identity keypair from storage
    virtual bool loadOrCreate(const QString& keyStoragePath,
                              const QString& displayName = QString()) = 0;

    // Cryptographic identity attributes
    virtual QString peerId() const = 0;  // e.g. "peer:ed25519:<hex>"
    virtual QString displayName() const = 0;
    virtual void setDisplayName(const QString& name) = 0;

    virtual QByteArray publicKey() const = 0;  // Ed25519 public key (32 bytes)
    virtual QByteArray secretKey() const = 0;  // Ed25519 secret key (64 bytes)

    // Sign arbitrary message data
    virtual QByteArray sign(const QByteArray& data) const = 0;

    // Validate a remote peer's signature against their Peer ID
    virtual bool verifyPeerSignature(const QString& peerId, const QByteArray& data,
                                     const QByteArray& signature) const = 0;

    // Helper: extract raw Ed25519 public key from a peerId string
    static QByteArray extractPublicKey(const QString& peerId);
    static QString formatPeerId(const QByteArray& publicKey);

signals:
    void identityChanged();
};

}  // namespace peerchat::identity

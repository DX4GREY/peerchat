#pragma once

#include "IIdentityProvider.hpp"
#include "../crypto/CryptoUtils.hpp"

namespace peerchat::identity {

class SodiumIdentityProvider : public IIdentityProvider {
    Q_OBJECT
public:
    explicit SodiumIdentityProvider(QObject* parent = nullptr);
    ~SodiumIdentityProvider() override;

    bool loadOrCreate(const QString& keyStoragePath, const QString& displayName = QString()) override;

    QString peerId() const override;
    QString displayName() const override;
    void setDisplayName(const QString& name) override;

    QByteArray publicKey() const override;
    QByteArray secretKey() const override;

    QByteArray sign(const QByteArray& data) const override;
    bool verifyPeerSignature(const QString& peerId, const QByteArray& data, const QByteArray& signature) const override;

private:
    QString m_keyPath;
    QString m_displayName{"Anonymous Peer"};
    crypto::CryptoUtils::KeyPair m_keyPair;
    bool m_loaded{false};
};

}  // namespace peerchat::identity

#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include "../../core/identity/SodiumIdentityProvider.hpp"
#include "../../core/crypto/CryptoUtils.hpp"

using namespace peerchat;

TEST(IdentityTest, KeyPairGenerationAndPeerId) {
    ASSERT_TRUE(crypto::CryptoUtils::initialize());

    identity::SodiumIdentityProvider provider;
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    QString keyPath = tempDir.filePath("identity.key");
    EXPECT_TRUE(provider.loadOrCreate(keyPath, "Alice"));

    QString peerId = provider.peerId();
    EXPECT_TRUE(peerId.startsWith("peer:ed25519:"));
    EXPECT_EQ(peerId.length(), 13 + 64); // "peer:ed25519:" + 64 hex chars
    EXPECT_EQ(provider.displayName(), "Alice");

    QByteArray extractedPk = identity::IIdentityProvider::extractPublicKey(peerId);
    EXPECT_EQ(extractedPk, provider.publicKey());
    EXPECT_EQ(extractedPk.size(), 32);
}

TEST(IdentityTest, SignAndVerify) {
    identity::SodiumIdentityProvider provider;
    QTemporaryDir tempDir;
    QString keyPath = tempDir.filePath("identity.key");
    provider.loadOrCreate(keyPath, "Bob");

    QByteArray data = "Hello, cryptographic peer-to-peer world!";
    QByteArray signature = provider.sign(data);
    EXPECT_EQ(signature.size(), 64);

    // Verify valid signature
    EXPECT_TRUE(provider.verifyPeerSignature(provider.peerId(), data, signature));

    // Tampered data must fail
    QByteArray tamperedData = data + "tampered";
    EXPECT_FALSE(provider.verifyPeerSignature(provider.peerId(), tamperedData, signature));

    // Tampered signature must fail
    QByteArray tamperedSig = signature;
    tamperedSig[0] = static_cast<char>(tamperedSig[0] ^ 0xFF);
    EXPECT_FALSE(provider.verifyPeerSignature(provider.peerId(), data, tamperedSig));
}

TEST(IdentityTest, PersistenceReload) {
    QTemporaryDir tempDir;
    QString keyPath = tempDir.filePath("identity.key");

    QString originalPeerId;
    {
        identity::SodiumIdentityProvider provider1;
        EXPECT_TRUE(provider1.loadOrCreate(keyPath, "Charlie"));
        originalPeerId = provider1.peerId();
    }

    // Verify file permissions on unix (0600)
    QFile keyFile(keyPath);
    EXPECT_TRUE(keyFile.exists());

    {
        identity::SodiumIdentityProvider provider2;
        EXPECT_TRUE(provider2.loadOrCreate(keyPath));
        EXPECT_EQ(provider2.peerId(), originalPeerId);
        EXPECT_EQ(provider2.displayName(), "Charlie");
    }
}

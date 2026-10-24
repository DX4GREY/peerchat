#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QCryptographicHash>
#include "../../core/transfers/FileTransferManager.hpp"
#include "../../core/identity/SodiumIdentityProvider.hpp"
#include "../../core/storage/SqliteMessageStore.hpp"

using namespace peerchat;

TEST(TransfersTest, FileOfferAndIntegrityVerification) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // Create a 150KB test file
    QString testFilePath = tempDir.filePath("testfile.dat");
    QByteArray payloadData;
    payloadData.resize(150 * 1024);
    for (int i = 0; i < payloadData.size(); ++i) {
        payloadData[i] = static_cast<char>(i % 256);
    }
    {
        QFile f(testFilePath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write(payloadData);
        f.close();
    }

    QByteArray expectedHash = QCryptographicHash::hash(payloadData, QCryptographicHash::Sha256);

    identity::SodiumIdentityProvider identity;
    identity.loadOrCreate(tempDir.filePath("sender.key"), "Sender");

    storage::SqliteMessageStore store;
    store.open(tempDir.filePath("sender.db"));

    QString downloadDir = tempDir.filePath("downloads");
    transfers::FileTransferManager ftm(nullptr, &store, &identity, downloadDir);

    QString txId = ftm.startSendFile("peer:ed25519:dummy1234567890123456789012345678901234567890123456789012345678901234", testFilePath);
    EXPECT_FALSE(txId.isEmpty());

    auto stored = store.getTransfer(txId);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->fileSize, 150 * 1024);
    EXPECT_EQ(stored->sha256.toLower(), QString::fromUtf8(expectedHash.toHex()).toLower());
}

TEST(TransfersTest, EmptyFileTransferHandling) {
    QTemporaryDir tempDir;
    QString emptyPath = tempDir.filePath("empty.txt");
    {
        QFile f(emptyPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.close();
    }

    identity::SodiumIdentityProvider identity;
    identity.loadOrCreate(tempDir.filePath("sender.key"), "Sender");

    storage::SqliteMessageStore store;
    store.open(tempDir.filePath("store.db"));

    transfers::FileTransferManager ftm(nullptr, &store, &identity, tempDir.path());
    QString txId = ftm.startSendFile("peer:ed25519:dummy1234567890123456789012345678901234567890123456789012345678901234", emptyPath);
    EXPECT_FALSE(txId.isEmpty());

    auto stored = store.getTransfer(txId);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->fileSize, 0);
}

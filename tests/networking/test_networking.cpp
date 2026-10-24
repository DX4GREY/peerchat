#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTimer>

#include "../../core/identity/SodiumIdentityProvider.hpp"
#include "../../core/messaging/MessagingEngine.hpp"
#include "../../core/networking/DirectTcpTransport.hpp"
#include "../../core/storage/SqliteMessageStore.hpp"
#include <gtest/gtest.h>

using namespace peerchat;

TEST(NetworkingTest, TwoPeersLoopbackConnectAndMessage) {
    QTemporaryDir dirAlice;
    QTemporaryDir dirBob;

    // Alice
    identity::SodiumIdentityProvider idAlice;
    idAlice.loadOrCreate(dirAlice.filePath("alice.key"), "Alice");
    storage::SqliteMessageStore storeAlice;
    storeAlice.open(dirAlice.filePath("alice.db"));
    networking::DirectTcpTransport transportAlice(&idAlice);
    ASSERT_TRUE(transportAlice.startListening(0));
    quint16 alicePort = transportAlice.listeningPort();
    ASSERT_GT(alicePort, 0);

    messaging::MessagingEngine engineAlice(&idAlice, &transportAlice, nullptr, nullptr, &storeAlice,
                                           nullptr);

    // Bob
    identity::SodiumIdentityProvider idBob;
    idBob.loadOrCreate(dirBob.filePath("bob.key"), "Bob");
    storage::SqliteMessageStore storeBob;
    storeBob.open(dirBob.filePath("bob.db"));
    networking::DirectTcpTransport transportBob(&idBob);
    ASSERT_TRUE(transportBob.startListening(0));

    messaging::MessagingEngine engineBob(&idBob, &transportBob, nullptr, nullptr, &storeBob,
                                         nullptr);

    bool bobReceivedMessage = false;
    QString receivedContent;
    QObject::connect(&engineBob, &messaging::MessagingEngine::messageReceived,
                     [&](const storage::StoredMessage& msg) {
                         bobReceivedMessage = true;
                         receivedContent = msg.content;
                     });

    bool aliceReceivedAck = false;
    QObject::connect(&engineAlice, &messaging::MessagingEngine::messageDelivered,
                     [&](const QString& msgId, qint64 time) {
                         Q_UNUSED(msgId);
                         Q_UNUSED(time);
                         aliceReceivedAck = true;
                     });

    // Bob connects to Alice over TCP loopback
    transportBob.connectToPeer(idAlice.peerId(), "127.0.0.1", alicePort);

    // Run event loop until connected, message sent, and delivered (timeout 3000ms)
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(3000);

    bool messageSent = false;

    while (timer.isActive()) {
        qApp->processEvents(QEventLoop::AllEvents, 50);

        if (transportBob.isConnected(idAlice.peerId()) &&
            transportAlice.isConnected(idBob.peerId()) && !messageSent) {
            engineAlice.sendTextMessage(idBob.peerId(), "Hello from Alice over loopback TCP!");
            messageSent = true;
        }

        if (bobReceivedMessage && aliceReceivedAck) {
            break;
        }
    }

    EXPECT_TRUE(transportBob.isConnected(idAlice.peerId()));
    EXPECT_TRUE(transportAlice.isConnected(idBob.peerId()));
    EXPECT_TRUE(messageSent);
    EXPECT_TRUE(bobReceivedMessage);
    EXPECT_EQ(receivedContent, "Hello from Alice over loopback TCP!");
    EXPECT_TRUE(aliceReceivedAck);
}

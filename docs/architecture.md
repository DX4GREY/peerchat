# PeerChat Architecture

## 1. System Overview

**PeerChat** is a cross-platform peer-to-peer (P2P) messaging and file-sharing application designed for Linux, Windows, and Android. It provides secure one-to-one messaging, attachment and arbitrary file transfer, verified cryptographic identity, local conversation history, and peer discovery with relay fallback.

### High-Level Architecture Diagram

```
+---------------------------------------------------------------------------------+
|                                 PeerChat UI                                     |
|               (Qt 6 Quick / QML / Material & Responsive Controls)               |
+---------------------------------------------------------------------------------+
                                      |
                                  Qt Signals
                                  & C++ Slots
                                      v
+---------------------------------------------------------------------------------+
|                           Application / ViewModel Layer                         |
|  - ChatController        - ContactListModel       - TransferQueueModel          |
|  - IdentityController    - DiagnosticsModel       - SettingsController          |
+---------------------------------------------------------------------------------+
                                      |
                                      v
+---------------------------------------------------------------------------------+
|                                  Core Domain                                    |
|                                                                                 |
|  +--------------------+  +--------------------+  +---------------------------+  |
|  |  IIdentityProvider |  |   IMessageStore    |  |       IFileTransfer       |  |
|  |  (SodiumIdentity)  |  |  (SqliteStore)     |  |  (ChunkedTransferManager) |  |
|  +--------------------+  +--------------------+  +---------------------------+  |
|             |                      |                           |                |
|             v                      v                           v                |
|  +---------------------------------------------------------------------------+  |
|  |                             Messaging Service                             |  |
|  |     (Packet framing, serialization, dispatch, delivery confirmation)      |  |
|  +---------------------------------------------------------------------------+  |
|                                     |                                           |
|                                     v                                           |
|  +--------------------+  +---------------------------------------------------+  |
|  |   IPeerDiscovery   |  |                  IPeerTransport                   |  |
|  | (LanMulticastDisc) |  | (Direct TCP/TLS Transport + RelayFallbackTransport)|  |
|  +--------------------+  +---------------------------------------------------+  |
+---------------------------------------------------------------------------------+
                                      |
                       +--------------+--------------+
                       |                             |
                       v                             v
             [ Local Network / Direct ]      [ Relay Service ]
             (Direct TCP/TLS or UDP)         (PeerChat Relay Daemon)
```

---

## 2. Networking Feasibility Assessment & ADR (Architecture Decision Record)

As mandated by Section 2 of `PROMPTS.md`, we evaluated three primary networking strategies:

### Option A: Qt/C++ application with libp2p integrated via FFI
- **Evaluation**:
  - *Native C++ libp2p (`cpp-libp2p`)*: Soramitsu's implementation is tied to Polkadot/Kagome toolchains, lacks official upstream multiplatform prebuilts, has incomplete Windows MSVC and Android NDK support, and has irregular releases.
  - *Go libp2p (`go-libp2p`)*: Actively maintained reference implementation. However, compiling Go with CGO for cross-platform targets (Android ARM64/x86_64, Windows MinGW/MSVC, Linux x86_64) requires heavy toolchains, adds 30MB+ per binary architecture, introduces CGO boundary latency, and creates thread-pool contention between Go's goroutine runtime and Qt's event loop.
  - *Rust libp2p (`rust-libp2p`)*: High code quality, but requires Rust toolchain on all developer and CI machines, complex FFI bindings (cxx/cbindgen), and cross-compilation setup for Android NDK.
- **Verdict**: Unsuitable as a rigid single-transport dependency for cross-platform Qt6 without risking build failure on consumer machines or Android NDK targets.

### Option B: Qt/C++ with WebRTC Native C++ (`libwebrtc` / `libdatachannel`)
- **Evaluation**:
  - Google's native `libwebrtc` requires Chromium depot_tools, 15+ GB checkouts, and custom build flags.
  - `libdatachannel` provides a clean standalone WebRTC DataChannel implementation (C++17/20, OpenSSL). It handles ICE, STUN, TURN, and DTLS-SCTP. However, it requires an external signaling channel to exchange SDP offers/answers.
- **Verdict**: Excellent long-term candidate for NAT traversal data channels when combined with a lightweight signaling relay.

### Option C: QtNetwork + libsodium Authenticated Transport with Relay Fallback (Selected Core Architecture)
- **Evaluation**:
  - Built natively on C++20, Qt6 Network (`QTcpSocket`, `QTcpServer`, `QUdpSocket`, `QSslSocket`), and `libsodium` (standard, auditable, cross-platform crypto library available on Linux, Windows, and Android).
  - Clean separation through interface boundaries (`IPeerTransport`, `IPeerDiscovery`, `IFileTransfer`, `IMessageStore`, `IIdentityProvider`).
  - Implements **Direct LAN/WAN TCP transport with cryptographic handshake**, **UDP multicast/broadcast zero-configuration discovery**, and **Authenticated Relay Service fallback** when direct NAT traversal is impossible.
  - Highly testable using mock transports in unit tests and loopback socket tests in CI without requiring third-party cloud infrastructure.

### Architectural Decision
We define clean interface contracts (`IPeerTransport`, `IPeerDiscovery`, `IFileTransfer`, `IMessageStore`, `IIdentityProvider`). The primary production transport provides **Direct TCP/TLS with Noise-like libsodium authenticated sessions** backed by an **Autonomous Relay Daemon** (`services/relay`) for NAT fallback. The transport abstraction allows swapping or augmenting with `libdatachannel` or `libp2p` without touching UI or business logic.

---

## 3. Core Interface Specifications

1. **`IIdentityProvider`**:
   - Manages cryptographic identity keys (Ed25519 signing + X25519 diffie-hellman key exchange via libsodium).
   - Generates and verifies Peer IDs formatted as base58/hex strings (`peer:<pubkey>`).
   - Securely persists keypairs in local storage.

2. **`IPeerTransport`**:
   - Asynchronous peer connection, listening, disconnect, and packet transmission.
   - Emits signals: `peerConnected(peerId, routeType)`, `peerDisconnected(peerId)`, `packetReceived(peerId, packet)`, `routeChanged(peerId, routeType)`.
   - Route types: `Direct`, `Relay`, `Connecting`, `Offline`.

3. **`IPeerDiscovery`**:
   - Local LAN discovery (multicast beacon / response) and Rendezvous relay announcement.
   - Discovers peer endpoints (`ip:port`, Peer ID) and feeds them into the transport layer.

4. **`IMessageStore`**:
   - Persistent SQLite engine storing contacts, conversations, messages, and transfer logs.
   - ACID-compliant, supports paging, status updates (Sending, Sent, Delivered, Read, Failed).

5. **`IFileTransfer`**:
   - Bounded chunk-based file transfer (64KB chunks).
   - SHA-256 hash calculation, temporary `.part` reassembly, resume verification, and path traversal sanitization.

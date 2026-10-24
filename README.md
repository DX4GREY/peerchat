# PeerChat

[![CI](https://github.com/peerchat/peerchat/actions/workflows/ci.yml/badge.svg)](https://github.com/peerchat/peerchat/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Qt6](https://img.shields.io/badge/GUI-Qt%206-41CD52.svg)](https://www.qt.io/)

PeerChat is a modern, secure, cross-platform peer-to-peer (P2P) messaging and file sharing application supporting **Linux**, **Windows**, and **Android**.

---

## Features

- **Direct P2P First**: Automatic zero-configuration peer discovery via LAN UDP multicast and direct point-to-point TCP connections.
- **Relay Fallback**: Dedicated, rate-limited relay server (`peerchat-relay`) for traversing strict symmetric NATs and CGNAT.
- **Cryptographic Identities**: Ed25519 signing keys and deterministic Peer IDs (`peer:ed25519:...`).
- **End-to-End Integrity**: Length-prefixed framed protocol, signed message payloads, and replay protection.
- **Resilient File Transfer**: Bounded 64 KB chunked streaming, SHA-256 whole-file cryptographic hash verification, atomic promotion from `.part` temporary files, and directory traversal defense.
- **Local Persistence**: ACID SQLite storage with WAL mode for contacts, message history, and transfer logs.
- **Modern Responsive UI**: Qt Quick / QML interface with dark/light themes, desktop split view, and mobile layout.

---

## Repository Structure

```
peerchat/
├── CMakeLists.txt
├── CMakePresets.json
├── CMake/
│   └── Dependencies.cmake
├── apps/
│   └── client/             # Qt 6 Quick / QML cross-platform application
│       ├── src/            # ViewModels & Controllers
│       ├── qml/            # QML Views & Components
│       └── platform/       # Android manifest and resources
├── core/                   # C++20 Core domain libraries (peerchat_core)
│   ├── crypto/             # libsodium wrappers & SHA-256
│   ├── identity/           # Ed25519 cryptographic identity provider
│   ├── networking/         # Direct TCP, FramedSocket, Relay client
│   ├── discovery/          # UDP multicast local peer discovery
│   ├── storage/            # SQLite message & contact database
│   ├── transfers/          # Chunked file transfer & hash verification
│   ├── messaging/          # Protocol envelope serialization & dispatch
│   └── diagnostics/        # RTT, bandwidth & connection monitoring
├── services/
│   └── relay/              # Standalone autonomous relay daemon
├── tests/                  # GoogleTest automated unit & integration tests
├── packaging/              # Packaging scripts for Linux, Windows, Android
├── docs/                   # Architectural Decision Records & Specs
└── .github/workflows/      # Automated CI/CD, security & release workflows
```

---

## Quick Start (Building Locally)

### 1. Prerequisites
- C++20 compiler (GCC 11+, Clang 14+, or MSVC 2022)
- CMake 3.22+ and Ninja
- Qt 6 (Core, Gui, Qml, Quick, QuickControls2, Network, Sql)
- libsodium & sqlite3

### 2. Configure & Build
```bash
# Configure using preset or direct cmake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPEERCHAT_BUILD_TESTS=ON

# Compile everything (core, client, relay, tests)
cmake --build build
```

### 3. Run Automated Tests
```bash
ctest --test-dir build --output-on-failure
```

### 4. Running Two Test Peers Locally
To test end-to-end communication on the same machine:

```bash
# Peer 1 (Alice)
./build/apps/client/PeerChat --profile alice --port 42421 --storage /tmp/alice

# Peer 2 (Bob)
./build/apps/client/PeerChat --profile bob --port 42422 --storage /tmp/bob
```

1. Alice and Bob will discover each other over LAN multicast.
2. Direct connection is established over loopback TCP.
3. Exchange text messages and verify real-time delivery acks (`✓✓`).
4. Transfer files or images with progress monitoring and SHA-256 verification.

---

## License

PeerChat is licensed under the [Apache License 2.0](LICENSE).

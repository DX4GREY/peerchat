# PeerChat Testing Guide

## 1. Automated Tests

The PeerChat test suite is divided into unit tests and integration tests:

| Test Suite | Location | Scope |
|---|---|---|
| `unit_tests` | `tests/unit/` | Identity, cryptographic keys, protocol serialization, sanitization, SQLite persistence |
| `networking_tests` | `tests/networking/` | Transport framing, handshake, loopback peer-to-peer messaging |
| `transfers_tests` | `tests/transfers/` | File chunking, hash verification, pause/resume, cancellation, error recovery |

### Running Tests Locally
```bash
# Build tests
cmake -B build -G Ninja -DPEERCHAT_BUILD_TESTS=ON
cmake --build build

# Run via ctest
ctest --test-dir build --output-on-failure
```

---

## 2. Multi-Peer Local Testing

To test real end-to-end communication between two instances on the same machine:

```bash
# Terminal 1: Launch Alice (port 42421, storage dir /tmp/alice)
./build/apps/client/PeerChat --profile alice --port 42421 --storage /tmp/alice

# Terminal 2: Launch Bob (port 42422, storage dir /tmp/bob)
./build/apps/client/PeerChat --profile bob --port 42422 --storage /tmp/bob
```

1. Alice and Bob will discover each other automatically via LAN UDP multicast.
2. Direct connection is established over TCP loopback.
3. Send text messages and observe real-time delivery acks (`Delivered`).
4. Attach an image or file and confirm chunk transfer and SHA-256 integrity match.

---

## 3. Manual Matrix Testing Checklists

- [ ] **Android to Windows**: Wi-Fi direct connection and file transfer.
- [ ] **Windows to Linux**: Text message bidirectional exchange with UTF-8 emojis.
- [ ] **Relay Fallback**: Block direct port via `iptables`/firewall; verify status switches to `Relay`.
- [ ] **Transfer Interruption**: Terminate peer mid-transfer; restart and verify resume from last chunk.
- [ ] **Malicious Payload Defense**: Verify filenames like `../../etc/passwd` are sanitized to `passwd`.

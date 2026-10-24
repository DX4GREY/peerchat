# PeerChat Networking Model

## 1. Connection Philosophy
PeerChat prioritizes **direct peer-to-peer connectivity**. When direct connectivity fails due to Symmetric NAT, carrier-grade NAT (CGNAT), or firewall policies, connections automatically fall back to an authenticated **Relay Server**.

The active route is transparently surfaced to the user via the UI:
- **Direct**: Point-to-point TCP/TLS socket over LAN or publicly reachable WAN IP.
- **Relay**: Routed securely through an authenticated relay daemon (`services/relay`).
- **Connecting**: Discovery or handshake in progress.
- **Offline**: No route or peer unreachable.

```
+-----------+                                           +-----------+
|  Peer A   | <---------- Direct TCP / Noise ---------> |  Peer B   |
+-----------+                                           +-----------+
      ^                                                       ^
      |                                                       |
      |   (If direct connection fails or blocked by CGNAT)   |
      +-------------> [ PeerChat Relay Daemon ] <-------------+
                        (services/relay)
```

---

## 2. Peer Discovery

### 2.1 Local Area Network (Zero-Configuration)
- Multicast UDP Beacon on `239.255.42.99:4242` (IPv4) or link-local broadcast.
- Periodic announcement contains:
  ```json
  {
    "proto": "peerchat/discovery/1.0",
    "peer_id": "peer:ed25519:...",
    "display_name": "Alice",
    "listen_port": 42420,
    "timestamp": 1729750000
  }
  ```
- Peers on the same Wi-Fi / Ethernet subnet immediately discover each other without external internet access or servers.

### 2.2 WAN Discovery & Rendezvous
- Configurable Relay/Rendezvous server endpoints in Settings (`relay.peerchat.local:42424`).
- Peers register their Peer ID, public IP/port candidates, and query known contact statuses.
- Hole punching attempts are initiated by exchanging candidate addresses via the rendezvous control channel.

---

## 3. Transport Security Handshake

1. Peer A establishes a raw TCP connection to Peer B (or to Relay).
2. Handshake Phase:
   - Ephemeral Key Exchange (X25519) combined with long-term Ed25519 authentication.
   - Server/Client nonces exchange.
   - Shared secret derivation via `crypto_kx` (libsodium).
   - Authenticated encryption (ChaCha20-Poly1305) on all subsequent framed messages.
3. If the handshake fails or the remote Peer ID does not match the signed public key, the connection is instantly aborted.

---

## 4. Relay Architecture & Security Safeguards

Section 3 of the specification prohibits unrestricted open relays:
- **Separately deployable service**: Located in `services/relay/`.
- **Authentication**: Relay connections authenticate with a shared token or pre-registered peer identities.
- **Rate limiting**: Maximum 50 messages/sec per peer; maximum 5 concurrent connections per IP.
- **Bandwidth caps**: Configurable maximum relay transfer rate to prevent abuse.
- **End-to-End Privacy**: The relay server forwards encrypted frames and cannot read payload contents.

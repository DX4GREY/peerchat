# PeerChat Security & Threat Model

## 1. Cryptographic Foundations
- **Identity & Signatures**: Ed25519 (`crypto_sign_ed25519` via libsodium). Peer IDs are deterministically derived from public keys.
- **Key Exchange**: X25519 diffie-hellman (`crypto_kx` / `crypto_scalarmult`).
- **Symmetric Encryption**: Authenticated ChaCha20-Poly1305 (`crypto_secretbox` / `crypto_aead`).
- **Hashing**: SHA-256 (`QCryptographicHash::Sha256`) and BLAKE2b (`crypto_generichash`).

---

## 2. Threat Model & Mitigations

### 2.1 Man-In-The-Middle (MITM)
- **Threat**: An adversary on the local network or untrusted relay intercepts or tampers with messages.
- **Mitigation**: End-to-end authenticated sessions. Senders sign message digests with their Ed25519 private key. The relay only forwards opaque ciphertexts and cannot decrypt or forge messages.

### 2.2 Malicious Payloads & Remote Code Execution
- **Threat**: An attacker sends crafted filenames, oversized buffers, or malicious files to compromise the host.
- **Mitigation**:
  1. Strict input validation of JSON schema and string lengths.
  2. Filenames are sanitized via `QFileInfo::fileName()` and regex stripping of all non-printable/path separator characters.
  3. No automatic execution or previewing of executable file types (`.exe`, `.sh`, `.apk`, `.so`, `.bat`).
  4. Memory bounds: Chunk size is fixed at 64 KB, preventing memory exhaustion attacks. Files are written directly to disk via buffered streams.

### 2.3 Identity Spoofing
- **Threat**: An attacker impersonates a known contact.
- **Mitigation**: A peer's identifier is bound to their public key. In the trust model, contacts store the pinned public key. Any change in public key triggers an untrusted key notification in the UI.

### 2.4 Denial of Service (DoS) & Relay Flooding
- **Threat**: Flooding peers or relays with excessive traffic.
- **Mitigation**: Per-connection rate limiting (max 50 msgs/sec), chunk throttling, connection limits per IP address on the relay daemon.

---

## 3. Storage Security
- Messages and contact keys are stored in a local SQLite database with restrictive OS file permissions (`0600` on Unix/Linux/Android).
- Ephemeral session keys are zeroized from RAM using `sodium_memzero` upon connection teardown.
- Private keys are stored in a protected keystore file (`identity.key`) using `0600` permissions.
- Sensitive contents (private keys, plaintext messages) are never written to application logs.

---

## 4. Known Limitations
- Metadata Privacy: Relay nodes observe message traffic frequency and approximate packet sizes (mitigated partially by chunk normalization).
- Background Mobile Connectivity: Direct P2P sockets may be paused by Android's Doze mode.

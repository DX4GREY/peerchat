# PeerChat Application Protocol Specification

**Protocol Version**: `1.0`

PeerChat employs a framed, binary/JSON bounded application protocol designed for low overhead, safe validation, and tamper resistance.

---

## 1. Frame Structure

Each transport frame on the wire consists of a 4-byte big-endian payload length header followed by the payload:

```
+-----------------------------------+---------------------------------------------+
| Length (4 bytes, unsigned 32-bit) | Payload (JSON / Binary bounded to max 10MB) |
+-----------------------------------+---------------------------------------------+
```

Maximum frame payload size is strictly enforced at **10 MB** (to allow chunk packets up to 64 KB plus overhead, with an absolute upper bound). Any frame exceeding this limit immediately terminates the connection.

---

## 2. Envelope Fields

Every control and data message conforms to the following schema:

| Field | Type | Description |
|---|---|---|
| `version` | Integer | Protocol version (must be `1`) |
| `id` | String (UUID) | Unique message identifier |
| `type` | String | Message type (see below) |
| `sender_id` | String | Peer ID of sender (`peer:ed25519:...`) |
| `recipient_id`| String | Target Peer ID |
| `timestamp` | Int64 | Epoch millisecond timestamp |
| `conversation_id` | String | Target conversation ID (defaults to sender/recipient pair) |
| `payload` | Object | Type-specific payload dictionary |
| `signature` | String (Hex/B64) | Ed25519 signature of the payload by `sender_id` |

---

## 3. Message Types

### 3.1 `text`
Regular one-to-one text message.
```json
{
  "version": 1,
  "id": "c1f7a0b3-1e52-4f01-9f93-01d0a514d872",
  "type": "text",
  "sender_id": "peer:ed25519:a1b2...",
  "recipient_id": "peer:ed25519:f9e8...",
  "timestamp": 1729751000123,
  "payload": {
    "text": "Hello, secure peer!"
  }
}
```

### 3.2 `delivery_ack`
Receipt confirmation sent back to the original sender.
```json
{
  "version": 1,
  "id": "e8a94b30-...",
  "type": "delivery_ack",
  "sender_id": "peer:ed25519:f9e8...",
  "recipient_id": "peer:ed25519:a1b2...",
  "timestamp": 1729751001000,
  "payload": {
    "ack_message_id": "c1f7a0b3-1e52-4f01-9f93-01d0a514d872",
    "status": "delivered"
  }
}
```

### 3.3 `file_offer`
Initiates a chunked file transfer (document or image).
```json
{
  "version": 1,
  "id": "9b3c4d5e-...",
  "type": "file_offer",
  "sender_id": "peer:ed25519:a1b2...",
  "recipient_id": "peer:ed25519:f9e8...",
  "timestamp": 1729751002000,
  "payload": {
    "transfer_id": "t-847291",
    "filename": "photo.jpg",
    "mime_type": "image/jpeg",
    "total_size": 204800,
    "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    "chunk_size": 65536,
    "total_chunks": 4
  }
}
```

### 3.4 `file_chunk`
Carries a single chunk of data (Base64 encoded payload, bounded to 64KB).
```json
{
  "version": 1,
  "id": "3f4a5b6c-...",
  "type": "file_chunk",
  "sender_id": "peer:ed25519:a1b2...",
  "recipient_id": "peer:ed25519:f9e8...",
  "timestamp": 1729751003000,
  "payload": {
    "transfer_id": "t-847291",
    "chunk_index": 0,
    "chunk_data": "<base64_data>",
    "chunk_sha256": "..."
  }
}
```

### 3.5 `file_control`
Control commands for transfer lifecycle: `accept`, `reject`, `pause`, `resume`, `cancel`, `complete`.

### 3.6 `typing` / `presence`
Ephemeral presence indicators. Dropped gracefully if connection is congested.

---

## 4. Input Validation & Defense in Depth
- **Size Bounds**: Individual text messages capped at 64 KB; file chunks capped at 64 KB.
- **Filename Sanitization**: Path traversal sequences (`..`, `/`, `\`, null bytes `%00`) are stripped.
- **Hash Verification**: Incomplete files are stored in `.part` files. Upon receiving all chunks, the file is hashed via SHA-256. If hash mismatches, the `.part` file is removed and transfer marked `Failed`.
- **Replay Protection**: The receiver maintains a sliding cache of recent message IDs and monotonically increasing sequence timestamps.

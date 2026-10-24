# PeerChat Deployment & Relay Guide

## 1. Standalone Relay Service Deployment

The PeerChat Relay Daemon (`peerchat-relay`) provides fallback connectivity for peers trapped behind symmetric NAT or restrictive carrier firewalls.

### Building the Relay
```bash
cmake -B build-relay -G Ninja -DPEERCHAT_BUILD_CLIENT=OFF -DPEERCHAT_BUILD_RELAY=ON
cmake --build build-relay --target peerchat-relay
```

### Running with Systemd
Create `/etc/systemd/system/peerchat-relay.service`:
```ini
[Unit]
Description=PeerChat P2P Relay Daemon
After=network.target

[Service]
Type=simple
User=peerchat
ExecStart=/usr/local/bin/peerchat-relay --port 42424 --auth-token YOUR_SECURE_TOKEN --max-clients 500
Restart=always
RestartSec=5
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable --now peerchat-relay
```

### Docker Deployment
```dockerfile
FROM alpine:3.19
RUN apk add --no-cache libstdc++ libsodium qt6-qtbase
COPY peerchat-relay /usr/local/bin/peerchat-relay
EXPOSE 42424/tcp 42424/udp
ENTRYPOINT ["/usr/local/bin/peerchat-relay", "--port", "42424"]
```

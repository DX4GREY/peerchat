# PeerChat Release & Distribution Process

## 1. Versioning Strategy
PeerChat follows Semantic Versioning 2.0.0 (`vMAJOR.MINOR.PATCH`):
- `v0.1.0`: Initial core release (direct P2P, chunked file transfer, identity, local storage).
- Tagging a release on `main`:
  ```bash
  git tag -a v0.1.0 -m "PeerChat v0.1.0 Release"
  git push origin v0.1.0
  ```

---

## 2. Release Artifacts Produced

1. **Linux**:
   - `PeerChat-v0.1.0-x86_64.AppImage` or tarball.
   - SHA-256 checksum file: `PeerChat-v0.1.0-linux-x86_64.tar.gz.sha256`.
2. **Windows**:
   - `PeerChat-v0.1.0-win64.zip` (self-contained with all Qt runtime DLLs).
   - SHA-256 checksum file: `PeerChat-v0.1.0-win64.zip.sha256`.
3. **Android**:
   - Unsigned debug APK: `PeerChat-v0.1.0-debug.apk` (generated in CI).
   - Signed release APK: `PeerChat-v0.1.0-release.apk` (generated only when repository secrets `ANDROID_KEYSTORE_BASE64` and `KEYSTORE_PASSWORD` are configured).

---

## 3. GitHub Actions Release Workflow
The release workflow (`.github/workflows/release.yml`) automatically executes on version tag push:
1. Validates test suite across Linux and Windows runners.
2. Compiles optimized Release builds.
3. Computes cryptographic SHA-256 checksums.
4. Generates changelog and publishes GitHub Release assets.

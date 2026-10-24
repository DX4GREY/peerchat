#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
OUTPUT_DIR="${2:-dist/linux}"

echo "==> Packaging Linux binaries from ${BUILD_DIR}..."
mkdir -p "${OUTPUT_DIR}"

if [[ ! -f "${BUILD_DIR}/apps/client/PeerChat" ]] || [[ ! -f "${BUILD_DIR}/services/relay/peerchat-relay" ]]; then
    echo "Error: Binaries not found in ${BUILD_DIR}. Please build Release targets first."
    exit 1
fi

cp "${BUILD_DIR}/apps/client/PeerChat" "${OUTPUT_DIR}/"
cp "${BUILD_DIR}/services/relay/peerchat-relay" "${OUTPUT_DIR}/"

tar -czvf "PeerChat-linux-x86_64.tar.gz" -C "${OUTPUT_DIR}" .
sha256sum "PeerChat-linux-x86_64.tar.gz" > "PeerChat-linux-x86_64.tar.gz.sha256"

echo "==> Packaging complete: PeerChat-linux-x86_64.tar.gz"

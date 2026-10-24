#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-android}"
ABI="${2:-arm64-v8a}"

echo "==> Building PeerChat Android APK (${ABI})..."

if [[ -z "${ANDROID_SDK_ROOT:-}" ]] || [[ -z "${ANDROID_NDK_ROOT:-}" ]]; then
    echo "Warning: ANDROID_SDK_ROOT or ANDROID_NDK_ROOT is not set."
fi

cmake -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_ABI="${ABI}" \
  -DANDROID_PLATFORM=android-26 \
  -DPEERCHAT_BUILD_TESTS=OFF \
  -DPEERCHAT_BUILD_RELAY=OFF

cmake --build "${BUILD_DIR}" --target PeerChat

echo "==> Android build completed in ${BUILD_DIR}."

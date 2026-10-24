# Building PeerChat

## Prerequisites

| Dependency | Minimum Version | Notes |
|---|---|---|
| C++ Compiler | C++20 compliant (GCC 11+, Clang 14+, MSVC 2022) | Linux: `g++` / `clang++` |
| CMake | 3.22+ | Build configuration |
| Ninja | 1.10+ | Recommended build tool |
| Qt 6 | 6.5+ (Qt 6.12+ tested) | Core, Gui, Quick, Qml, Network, Sql, Test |
| libsodium | 1.0.18+ | High-speed cryptographic primitives |
| SQLite3 | 3.35+ | Message and contact storage |
| GoogleTest | 1.12+ | Automated unit and integration testing |

---

## 1. Linux Build

### Install Dependencies (Debian / Ubuntu / Arch)
```bash
# Ubuntu / Debian
sudo apt update && sudo apt install -y build-essential cmake ninja-build \
  qt6-base-dev qt6-declarative-dev libsodium-dev libsqlite3-dev libgtest-dev

# Arch Linux
sudo pacman -S base-devel cmake ninja qt6-base qt6-declarative libsodium sqlite gtest
```

### Configure and Build
```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Or manually:
```bash
mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPEERCHAT_BUILD_TESTS=ON
ninja
```

---

## 2. Windows Build (MSVC / MinGW)

### Prerequisites
- Visual Studio 2022 (MSVC v143+) or MinGW-w64 12+
- Qt 6.5+ for MSVC or MinGW installed via Qt Online Installer
- CMake 3.22+ and Ninja

### Build with CMake
```powershell
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.6.0\msvc2022_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Packaging Windows Binaries
Run `windeployqt` to bundle required Qt DLLs:
```powershell
windeployqt.exe build/apps/client/PeerChat.exe --qmldir apps/client/qml
```

---

## 3. Android Build

### Prerequisites
- Android Studio / Command Line Tools
- Android SDK (API 34 / Android 14 recommended, min API 26)
- Android NDK (r25c or newer)
- JDK 17
- Qt 6 for Android (e.g., `arm64-v8a`)

### Build Command
```bash
export ANDROID_SDK_ROOT=/path/to/android-sdk
export ANDROID_NDK_ROOT=/path/to/android-sdk/ndk/25.2.9519653

cmake -B build-android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DQT_HOST_PATH=/usr/lib/qt6 \
  -DCMAKE_PREFIX_PATH=/path/to/qt6-android \
  -DPEERCHAT_BUILD_TESTS=OFF

ninja -C build-android apk
```

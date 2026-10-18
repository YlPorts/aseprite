# Aseprite — experimental native Android port

This directory contains Android platform code for the **original C++ Aseprite**.
It is NOT a replacement editor, a WebView, or a runnable APK.

## Verified locally

- C++17 gesture router: brush start/move/end, pressure, pen/eraser,
  two-finger pan/zoom, cancel handling and ghost-stroke suppression.
- Blocking FIFO native event queue kernel with timeout/shutdown handling.
- Linux host test suite: **2/2 passed** in the implementation environment.

## Run tests

```sh
bash android-port/scripts/test-host.sh
```

## ARM64 native module (NDK)

```sh
cmake -S android-port -B build/aseprite-android-arm64 -G Ninja \
 -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26
cmake --build build/aseprite-android-arm64 --target aseprite-android-touch
```

The GitHub Actions workflow validates the native module, not a complete APK.

## Original LAF integration

The platform adapter targets genuine `os::Event` and `os::EventQueue`.
An idempotent source patch is in `scripts/patch-laf-headless-android.py`.
It adds a **headless** Android event queue to the LAF submodule. The
original `WindowAndroid` and Skia renderer still need to be implemented.

**Do not label this a complete port or publish Android Aseprite binaries.**
The containing fork is currently public; switch it to private in GitHub
Settings → General → Danger Zone → Change repository visibility before
any private builds/artifacts.

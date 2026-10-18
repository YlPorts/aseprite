# Aseprite — experimental native Android port

This directory contains Android platform code for the **original C++ Aseprite**.
It is NOT a replacement editor, a WebView, or a runnable APK. The original
editor Java/JNI host and Aseprite app_main() linker target are now present,
but they are **not validated as a working Android application**.

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

The GitHub Actions workflow **attempts to validate** the native module.
Do not report ARM64 success until its run has been inspected. It does not
build a complete APK.

## Original LAF integration

The platform adapter targets genuine `os::Event` and `os::EventQueue`.
The fail-closed source patch is in `scripts/patch-laf-headless-android.py`.
It adds a headless Android event queue to a checkout of the original LAF
submodule. Additional guarded patch scripts add experimental native Android
`WindowAndroid`, `SystemAndroid`, Skia raster presenter, Java Activity,
and original `app_main()` JNI entry. **These sources have not yet passed a
complete Skia+app-lib Android build.** See `docs/BUILD_ANDROID.md`.

## License and repository visibility

Aseprite's EULA restricts compiled copies to personal use or contributions
and prohibits distributing the application. Do not publish Aseprite APKs.
This fork is currently **public**. GitHub does not allow a public fork's
visibility to be changed independently. For private development, create a
separate private repository (not a GitHub fork), migrate this work into it,
and restrict CI artifacts to that private repository.

This branch includes platform source and tests only; it publishes no APK.

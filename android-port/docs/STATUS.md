# Android port status — work in progress

Source: original `YlPorts/aseprite` fork of `aseprite/aseprite`.
Branch: `android-native-port-wip`.

## Present in source
- C++ touch gesture arbitration and stylus/pressure paths.
- NDK `AInputEvent` decoder, not yet attached to Android's Activity.
- Translation to existing LAF `os::Event` mouse/space/magnify events.
- Thread-safe Android LAF event queue implementation and headless patch.
- CMake builds for Linux host tests and Android NDK ARM64 module.
- No new drawing engine or alternative interface.

## Verified
- Host test build and 2/2 CTest cases passed locally.
- Android NDK CI result is pending; do not claim it succeeded.
- No complete Aseprite compile, launch, or APK has been tested.

## Missing critical subsystems
1. `os::SystemAndroid`, `os::WindowAndroid`, surface lifecycle and resize.
2. Skia Android GPU/raster backend and pixel-perfect presentation.
3. Original Aseprite `app_main()` integration with NativeActivity/Java.
4. Build `gen` on Ubuntu host, cross-compile and link original `app-lib`.
5. Android SAF file dialogs and content-URI handling, autosave/recovery.
6. Text IME, hardware keys/mouse, app suspend/resume, DPI/insets.
7. End-to-end verification of all original tools/formats/timeline.
8. APK packaging, signing, device tests on Mali-G57 MC2.

## License and privacy

Aseprite's EULA permits compiling/modifying source for personal use or to
propose a contribution and restricts distribution. The public fork must not
publish a complete Aseprite APK. Keep private build artifacts private.

This branch only adds separate native platform-layer source and tests.

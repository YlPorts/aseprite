# Aseprite core on Android — experimental 0.1

This is a **partial native port of the Aseprite document/render/format engine**, with an Android touchscreen frontend. It is NOT the complete desktop Aseprite application, a WebView imitation, a Windows emulator, or an official Igara mobile release.

Target: ARM64 Android 8.0/API 26 and newer. No root, Internet permission, or all-files storage permission. Source baseline: upstream commit `375989a61c3425cd4e8cdedfcfcca4bdfef7e1d9`.

## Implemented in this first source revision

- The actual upstream C++ `doc-lib`, `render-lib`, and `dio-lib`, linked through the Android NDK and JNI.
- Transparent RGBA documents; pencil/eraser with integer-pixel strokes; brush size and hexadecimal colors.
- Original Aseprite rendering and ASEPRITE encoding/decoding, bounded undo/redo history.
- Image layers, layer selection/visibility, frame duplication/selection/duration, animation preview.
- Pinch zoom and two-finger pan; Android document picker to open `.aseprite`, save a new project copy, or export the current frame as transparent PNG.
- Best-effort local session recovery when pausing. Always save your own copies externally.
- Native tests for drawing, color order, encoding roundtrip, undo/redo, layer compositing, and independent duplicate frames.

**These are source implementations, not a claim of full on-device testing.** Check the workflow result before treating an APK as available. Linux core tests are not a substitute for running it on an Android phone.

## Not yet ported

The desktop interface, complete tool system, selections/transforms, tilemaps, reference layers, grayscale/indexed editing, Lua extensions, GIF export, pressure-sensitive stylus and full desktop keyboard behavior. Some advanced project metadata may not be editable. For testing, use copies of ordinary RGBA projects, not your only original.

Limits: new canvases up to 512×512; imported canvases up to 1024×1024; files up to 32 MB; up to 128 frames/64 image layers, subject to memory limits. This initial version intentionally rejects unsupported document types instead of silently flattening them.

## Build

Clone this branch with `git clone --recursive`; a GitHub source ZIP does not include the submodules.
Install Java 17, Gradle 8.9, SDK 35, NDK 27.2.12479018 and CMake 3.22.1. Then run `gradle -p android :app:assembleDebug`.

Host tests: `cmake -S android/app/src/main/cpp -B build-mobile-tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5`, then `cmake --build build-mobile-tests --target core-tests`, then `ctest --test-dir build-mobile-tests --output-on-failure`.

The branch-specific workflow builds **Android only**, and does not launch the previous Windows matrix. The existing Windows branch has been left as history.

Debug APK signing is for testing. A stable private release signing key must be set up before promising in-place upgrades between different CI builds. Do not uninstall an old build before saving project copies; uninstalling removes app-private recovery data.

Upstream module copyrights and licenses remain intact. The document, render, dio and LAF modules carry their own license notices, included as APK assets by the build. The desktop application's EULA is not being replaced or relicensed.

# Original Aseprite Android: native build integration (experimental)

**This is NOT an installable application yet.** The original C++ Aseprite GUI
must compile and link with Android-built **Skia m124**, the pinned laf submodule,
and its own asset directory. No substitution of editors is permitted.

## Sources and patch order

Use `YlPorts/aseprite`, branch `android-native-port-wip`. Clone recursively
in a private development environment. In the source root:

```sh
python3 android-port/scripts/patch-laf-headless-android.py .
python3 android-port/scripts/patch-laf-android-raster.py .
python3 android-port/scripts/patch-laf-android-runtime.py .
python3 android-port/scripts/patch-aseprite-android-engine.py .
```

Patch scripts are designed to fail when upstream anchors change, rather than
silently substituting missing code. The integration test clones required files
into a temporary directory and runs the complete overlay twice:

```sh
python3 android-port/tests/patch_laf_integration_test.py
```

GitHub Actions includes a preflight job for this test; a passing result has
not yet been confirmed. Do not mistake successful patching for a GUI build.

## Build requirements

- CMake >= 3.20, Ninja, Clang and Android NDK 27.2.12479018.
- Original Aseprite submodules, including `laf`.
- Original Aseprite's `gen` generator first built on Ubuntu x86_64.
- An **Android ARM64 build of Skia**, branch `aseprite-m124` (not the
  precompiled Linux, Windows or macOS Skia libraries).
- Original CMake dependencies such as zlib, libpng, freetype/harfbuzz.

## Skia m124 ARM64 experimental build

The exact Skia branch required by Aseprite is `aseprite-m124`. This port uses
its software raster backend first, not EGL/Vulkan acceleration. The experimental
workflow [android-skia-arm64-experimental.yml](../../.github/workflows/android-skia-arm64-experimental.yml)
now tries to build genuine `libskia.a`, `libskshaper.a`, and `libskunicode.a`
for Android ARM64, with the installed NDK and GN. After those libraries,
its experimental final step attempts to link the **original Aseprite** editor
into `libaseprite.so` using `scripts/try-build-original-editor-arm64.sh`.
No passing end-to-end run has been confirmed. The workflow does **not**
upload or distribute an APK or library.

Do not reuse precompiled Linux x86_64 Skia libraries for Android. The Android
Skia build must produce compatible ARM64 libraries and headers before GUI
linking can be tested.

Native engine configuration **when Skia Android and all dependencies exist**:

```sh
cmake -S . -B build/android-gui -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DENABLE_ASEPRITE_EXE=OFF -DASEPRITE_ANDROID_GUI=ON \
  -DLAF_BACKEND=skia -DSKIA_DIR=/path/to/android-skia \
  -DSKIA_LIBRARY_DIR=/path/to/android-skia/out/Release-Android-arm64 \
  -DGEN_EXE=/path/to/ubuntu-host/bin/gen \
  -DENABLE_NEWS=OFF -DENABLE_UPDATER=OFF \
  -DLAF_WITH_EXAMPLES=OFF -DLAF_WITH_TESTS=OFF
cmake --build build/android-gui --target aseprite
```

The expected real engine output is a shared `libaseprite.so`, not a
replacement rendering shell. Configuration/compilation success is not
yet verified. The original engine may require more Android portability fixes.

## Android packaging gate

Only after the real engine is linked, stage:

- Real `libaseprite.so` in `android-port/app/src/main/jniLibs/arm64-v8a/`.
- Genuine Aseprite original `data/` contents (theme, GUI XML, fonts, icons)
  as `android-port/app/src/main/assets/data/`.

Gradle deliberately FAILS if either the engine or `data/gui.xml` is missing.
This prevents a blank APK from being presented as a successful port.

Keep all binaries and build artifacts private. Do not publish APKs in this
public fork. The code has **not** been tested on Galaxy A15 yet.

## Platform fixes (October 2026)

- The original Skia CMake dispatch selects `skia_window_android.cpp` under
  `ANDROID`, instead of incorrectly adding the Linux/X11 window implementation.
- Repeated LAF overlay application recognizes later, more complete patches.
- The JNI-created window reference is released before the original
  `SkiaSystem` tears down the application event queue.
- Aseprite's `System::mousePosition()` now receives Android touch coordinates.
- Original `SkiaSystem::setTextInput()` can call an Android-window method,
  though a complete soft-keyboard/IME adapter is still missing.
- The original JNI shared target links the raster code once, not twice.

## Attempt the original JNI editor library in one GitHub job

Run `Build Skia and attempt original Aseprite Android ARM64 (experimental)`
under GitHub Actions on the `android-native-port-wip` branch. The job:

1. Builds Skia m124 as Android ARM64 software raster, and validates GN target OS/CPU.
2. Applies the original-source Android LAF overlays, checking idempotence.
3. Compiles the original `gen` utility for the Ubuntu host.
4. Cross-configures and links the original Aseprite `app_main()`, `app-lib`,
   `laf-os`, `doc-lib`, `render-lib`, and format modules into `libaseprite.so`.
5. Refuses to report success without a real nonempty shared library.

**This is a compilation attempt**, not a demonstrated working port. The CMake
configuration or build may expose additional portability errors. No APK is
created. Keep Aseprite binaries private for personal use under the EULA.

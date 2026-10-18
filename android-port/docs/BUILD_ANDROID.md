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
silently substituting missing code. GitHub Actions includes a preflight job.

## Build requirements

- CMake >= 3.20, Ninja, Clang and Android NDK 27.2.12479018.
- Original Aseprite submodules, including `laf`.
- Original Aseprite's `gen` generator first built on Ubuntu x86_64.
- An **Android ARM64 build of Skia**, branch `aseprite-m124` (not the
  precompiled Linux, Windows or macOS Skia libraries).
- Original CMake dependencies such as zlib, libpng, freetype/harfbuzz.

Native engine configuration **when Skia Android and all dependencies exist**:

```sh
cmake -S . -B build/android-gui -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DENABLE_ASEPRITE_EXE=OFF -DASEPRITE_ANDROID_GUI=ON \
  -DLAF_BACKEND=skia -DSKIA_DIR=/path/to/android-skia \
  -DSKIA_LIBRARY_DIR=/path/to/android-skia/out/android-arm64 \
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

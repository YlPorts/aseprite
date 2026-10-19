#!/usr/bin/env bash
# EXPERIMENTAL: attempts a real original Aseprite -> Android ARM64 shared build.
# Fail closed. No dummy .so, fake editor, or APK is generated.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-${ANDROID_HOME:-}/ndk/27.2.12479018}}"
SKIA="${ASEPRITE_ANDROID_SKIA_DIR:-$ROOT/build/skia}"
LIBS="${ASEPRITE_ANDROID_SKIA_LIBS:-$SKIA/out/Release-Android-arm64}"
GEN_BUILD="$ROOT/build/aseprite-hostgen-android"
ANDROID_BUILD="$ROOT/build/aseprite-gui-android-arm64"
cd "$ROOT"
if [[ ! -f laf/os/CMakeLists.txt || ! -f laf/base/CMakeLists.txt || ! -f laf/clip/CMakeLists.txt ]]; then
  echo "ERROR: original pinned LAF submodules missing; git submodule update --init --recursive" >&2
  exit 2
fi
if [[ ! -f "$NDK/build/cmake/android.toolchain.cmake" ]]; then
  echo "ERROR: Android NDK 27.2.12479018 is missing" >&2
  exit 2
fi
if [[ ! -s "$LIBS/libskia.a" || ! -s "$LIBS/libskshaper.a" || ! -s "$LIBS/libskunicode.a" ]]; then
  echo "ERROR: genuine Android-built Aseprite Skia m124 static libraries are missing" >&2
  exit 2
fi
if ! grep -Eq '^[[:space:]]*target_os[[:space:]]*=[[:space:]]*"android"([[:space:]]|$)' "$LIBS/args.gn"; then
  echo "ERROR: provided Skia build is not targeting Android" >&2
  exit 2
fi
if ! grep -Eq '^[[:space:]]*target_cpu[[:space:]]*=[[:space:]]*"arm64"([[:space:]]|$)' "$LIBS/args.gn"; then
  echo "ERROR: provided Skia build is not ARM64" >&2
  exit 2
fi

python3 android-port/tests/patch_laf_integration_test.py
python3 android-port/scripts/patch-laf-headless-android.py .
python3 android-port/scripts/patch-laf-android-raster.py .
python3 android-port/scripts/patch-laf-android-runtime.py .
python3 android-port/scripts/patch-aseprite-android-engine.py .

# The genuine 'gen' executable is a host binary. Do not attempt to run
# an ARM64 Android generator on the Ubuntu runner.
cmake -S . -B "$GEN_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DLAF_BACKEND=none \
  -DENABLE_ASEPRITE_EXE=OFF -DENABLE_TESTS=OFF \
  -DENABLE_NEWS=OFF -DENABLE_UPDATER=OFF \
  -DENABLE_I18N_STRINGS=OFF -DENABLE_SCRIPTING=OFF -DENABLE_WEBSOCKET=OFF \
  -DLAF_WITH_TESTS=OFF -DLAF_WITH_EXAMPLES=OFF -DLAF_WITH_CLIP=OFF
cmake --build "$GEN_BUILD" --target gen -j 2
GEN="$GEN_BUILD/bin/gen"
if [[ ! -x "$GEN" ]]; then
  echo "ERROR: authentic Aseprite host generator was not produced" >&2
  exit 2
fi

cmake -S . -B "$ANDROID_BUILD" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DENABLE_ASEPRITE_EXE=OFF -DASEPRITE_ANDROID_GUI=ON \
  -DENABLE_TESTS=OFF -DENABLE_NEWS=OFF -DENABLE_UPDATER=OFF \
  -DENABLE_I18N_STRINGS=OFF -DENABLE_SCRIPTING=OFF -DENABLE_WEBSOCKET=OFF \
  -DLAF_WITH_EXAMPLES=OFF -DLAF_WITH_TESTS=OFF \
  -DLAF_WITH_CLIP=ON -DLAF_BACKEND=skia \
  -DGEN_EXE="$GEN" -DSKIA_DIR="$SKIA" -DSKIA_LIBRARY_DIR="$LIBS" \
  -DSKIA_LIBRARY="$LIBS/libskia.a" \
  -DSKUNICODE_LIBRARY="$LIBS/libskunicode.a" \
  -DSKSHAPER_LIBRARY="$LIBS/libskshaper.a"
echo "ASEPRITE_ANDROID_STAGE: Building original laf-os for ARM64"
cmake --build "$ANDROID_BUILD" --target laf-os -j 2
echo "ASEPRITE_ANDROID_STAGE: laf-os complete; building original app-lib"
cmake --build "$ANDROID_BUILD" --target app-lib -j 2
echo "ASEPRITE_ANDROID_STAGE: app-lib complete; linking genuine libaseprite.so"
cmake --build "$ANDROID_BUILD" --target aseprite -j 2
echo "ASEPRITE_ANDROID_STAGE: original Android linker completed"

OUTPUT="$ANDROID_BUILD/lib/libaseprite.so"
if [[ ! -s "$OUTPUT" ]]; then
  OUTPUT="$(find "$ANDROID_BUILD" -type f -name 'libaseprite.so' -print -quit)"
fi
if [[ -z "$OUTPUT" || ! -s "$OUTPUT" ]]; then
  echo "ERROR: authentic original editor did not produce libaseprite.so" >&2
  exit 3
fi
READELF="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf"
if [[ ! -x "$READELF" ]] || ! "$READELF" -h "$OUTPUT" | grep -q "Machine:.*AArch64"; then
  echo "ERROR: linked library is not a real AArch64 ELF shared object" >&2
  exit 3
fi
# Java calls these exact functions through System.loadLibrary("aseprite").
# Check the real ELF dynamic symbol table, not just file names or strings.
for symbol in \
  Java_com_ylports_aseprite_AsepriteActivity_nativeStartOriginalEditor \
  Java_com_ylports_aseprite_AsepriteActivity_nativeAttachSurface \
  Java_com_ylports_aseprite_AsepriteActivity_nativeRegisterImeBridge \
  Java_com_ylports_aseprite_AsepriteActivity_nativeCommitText \
  Java_com_ylports_aseprite_AsepriteActivity_nativeTouch \
  Java_com_ylports_aseprite_AsepriteActivity_nativeKey \
  Java_com_ylports_aseprite_AsepriteActivity_nativeAppFocus
do
  if ! "$READELF" --dyn-syms --wide "$OUTPUT" | grep -Eq "[[:space:]]$symbol$"; then
    echo "ERROR: required original Android JNI entrypoint is not exported: $symbol" >&2
    exit 3
  fi
done
if "$READELF" -d "$OUTPUT" | grep -Fq 'Shared library: [libc++_shared.so]'; then
  echo "ERROR: editor unexpectedly depends on libc++_shared.so; use -DANDROID_STL=c++_static or package the runtime" >&2
  exit 3
fi
echo "Original Aseprite Android AArch64 shared-library linked with all seven JNI entrypoints: $OUTPUT."
echo "No APK was packaged, signed, uploaded, or distributed."

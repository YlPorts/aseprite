#!/usr/bin/env bash
# Inspect only authentic generated Skia libraries; no distribution or APK.
set -euo pipefail
LIB="${1:?usage: audit-original-skia-arm64.sh /path/to/Skia/out/Release-Android-arm64}"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-${ANDROID_HOME:-}/ndk/27.2.12479018}}"
READELF="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf"
AR="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar"
test -f "$LIB/args.gn"
test -x "$READELF"
test -x "$AR"
grep -Eq '^[[:space:]]*target_os[[:space:]]*=[[:space:]]*"android"' "$LIB/args.gn"
grep -Eq '^[[:space:]]*target_cpu[[:space:]]*=[[:space:]]*"arm64"' "$LIB/args.gn"
echo "GN source target: Android arm64"
echo "Compiled static archives (including third-party dependencies):"
find "$LIB" -maxdepth 2 -type f -name '*.a' -printf '%P\n' | sort
for name in libskia.a libskshaper.a libskunicode.a; do
  archive="$LIB/$name"
  test -s "$archive" || { echo "MISSING: $archive" >&2; exit 2; }
  member="$("$AR" t "$archive" | head -n 1)"
  [[ -n "$member" ]] || { echo "EMPTY ARCHIVE: $archive" >&2; exit 2; }
  # Every essential Skia archive must contain Android ARM64 relocatable
  # objects; file names alone do not prove architecture.
  header="$("$AR" p "$archive" "$member" | "$READELF" -h - 2>&1)" || {
    echo "CANNOT INSPECT $archive/$member" >&2
    exit 2
  }
  grep -q 'Machine:.*AArch64' <<< "$header" || {
    echo "WRONG ARCH: $archive member $member" >&2
    exit 2
  }
  echo "CHECKED: $name ($member) is AArch64"
done
echo "The Skia source archives passed architecture checks; this is not an Aseprite editor link."

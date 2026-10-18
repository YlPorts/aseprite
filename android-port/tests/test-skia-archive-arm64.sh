#!/usr/bin/env bash
# Positive/negative regression tests for real GN Skia archive architecture
# diagnostics. Synthetic fixtures test the auditor, NOT the Aseprite engine.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
AUDIT="$ROOT/android-port/scripts/audit-original-skia-arm64.sh"
CLANG="$(command -v clang)"
AR="$(command -v llvm-ar)"
READELF="$(command -v readelf)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
printf 'target_os = "android"\ntarget_cpu = "arm64"\n' > "$TMP/args.gn"
printf 'int original_skia_arch_probe(void) { return 7; }\n' > "$TMP/probe.c"
"$CLANG" --target=aarch64-linux-gnu -c "$TMP/probe.c" -o "$TMP/arm64.o"
"$CLANG" -c "$TMP/probe.c" -o "$TMP/host.o"
for lib in skia skshaper skunicode; do
  "$AR" rcs "$TMP/lib$lib.a" "$TMP/arm64.o"
done
export ASEPRITE_AR="$AR"
export ASEPRITE_READELF="$READELF"
bash "$AUDIT" "$TMP" >/dev/null
"$AR" rcs "$TMP/libskunicode.a" "$TMP/host.o"
if bash "$AUDIT" "$TMP" >/dev/null 2>&1; then
  echo "FAIL: wrong CPU archive was accepted" >&2
  exit 1
fi
"$AR" rcs "$TMP/libskunicode.a" "$TMP/arm64.o"
printf 'target_os = "linux"\ntarget_cpu = "arm64"\n' > "$TMP/args.gn"
if bash "$AUDIT" "$TMP" >/dev/null 2>&1; then
  echo "FAIL: Skia target_os linux was accepted as Android" >&2
  exit 1
fi
printf 'target_os = "android"\ntarget_cpu = "arm64"\n' > "$TMP/args.gn"
rm "$TMP/libskshaper.a"
if bash "$AUDIT" "$TMP" >/dev/null 2>&1; then
  echo "FAIL: missing original skshaper archive was accepted" >&2
  exit 1
fi
echo "PASS: Android ARM64 archive auditor accepts genuine AArch64 fixtures and rejects wrong CPU, OS and missing Skia modules."

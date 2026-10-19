#!/usr/bin/env python3
"""Stage ORIGINAL Aseprite resources and a verified linked Android ARM64 engine.

Fail closed: a JNI function name embedded in a binary is insufficient.
Only real, exported ELF dynamic symbols qualify. This does not publish an APK.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

ELF_MAGIC = b"\x7fELF"
EM_AARCH64 = 183
JNI_PREFIX = "Java_com_ylports_aseprite_AsepriteActivity_"
REQUIRED_JNI = (
    "nativeStartOriginalEditor", "nativeAttachSurface", "nativeRegisterImeBridge",
    "nativeCommitText", "nativeTouch", "nativeKey", "nativeAppFocus",
)


def readelf_tool(explicit=None):
    if explicit:
        return str(explicit)
    ndk = os.environ.get("ANDROID_NDK_HOME") or os.environ.get("ANDROID_NDK_ROOT")
    candidates = []
    if ndk:
        candidates.append(str(Path(ndk) / "toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf"))
    candidates.extend(["llvm-readelf", "readelf"])
    for candidate in candidates:
        resolved = shutil.which(candidate)
        if resolved:
            return resolved
    raise ValueError("No ELF inspection tool found (install binutils or Android NDK)")


def run_readelf(tool, *args):
    result = subprocess.run([tool, *args], capture_output=True, text=True, check=False)
    if result.returncode:
        raise ValueError(f"ELF symbol inspection failed: {result.stderr.strip()[:300]}")
    return result.stdout


def verify_aarch64_shared_lib(path: Path, readelf=None) -> None:
    with path.open("rb") as stream:
        header = stream.read(64)
    if len(header) < 64 or header[:4] != ELF_MAGIC:
        raise ValueError(f"{path}: not an ELF shared library")
    if header[4] != 2 or header[5] != 1:
        raise ValueError(f"{path}: expected 64-bit little-endian ELF")
    if struct.unpack_from("<H", header, 16)[0] != 3:
        raise ValueError(f"{path}: expected ET_DYN shared library")
    if struct.unpack_from("<H", header, 18)[0] != EM_AARCH64:
        raise ValueError(f"{path}: expected Android AArch64 CPU")
    if path.stat().st_size < 1024:
        raise ValueError(f"{path}: too small for the genuine Aseprite editor")

    tool = readelf_tool(readelf)
    symbols = run_readelf(tool, "--dyn-syms", "--wide", str(path))
    defined = set()
    for line in symbols.splitlines():
        columns = line.split()
        # Num: Value Size Type Bind Vis Ndx Name
        if len(columns) >= 8 and columns[0].endswith(":") and \
                columns[4] in ("GLOBAL", "WEAK") and columns[6] != "UND":
            defined.add(columns[7].split("@", 1)[0])
    absent = [JNI_PREFIX + name for name in REQUIRED_JNI
              if JNI_PREFIX + name not in defined]
    if absent:
        raise ValueError(f"{path}: missing exported JNI entrypoints: {', '.join(absent)}")

    dynamic = run_readelf(tool, "-d", str(path))
    dependencies = re.findall(r"Shared library: \[([^]]+)\]", dynamic)
    unsupported = [name for name in dependencies if name in (
        "libc++_shared.so", "libstdc++.so.6", "libc.so.6", "libX11.so.6")]
    if unsupported:
        raise ValueError(f"{path}: unexpected unpackaged/desktop dependencies: {unsupported}")


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument("--engine", type=Path, required=True)
    p.add_argument("--destination", type=Path)
    p.add_argument("--verify-only", action="store_true",
                   help="Inspect the linked ELF without copying any assets or library")
    args = p.parse_args()
    root = args.root.resolve()
    original_data = root / "data"
    destination = args.destination or root / "android-port/app/src/main"
    target_data = destination / "assets/data"
    native_out = destination / "jniLibs/arm64-v8a/libaseprite.so"

    if not args.engine.is_file():
        raise ValueError("Missing original engine output libaseprite.so")
    verify_aarch64_shared_lib(args.engine)
    if args.verify_only:
        print(f"Verified AArch64 ET_DYN, seven defined JNI exports and native dependencies: {args.engine}")
        return

    needed = ("gui.xml", "fonts", "icons/ase256.png")
    if not original_data.is_dir() or any(not (original_data / item).exists() for item in needed):
        raise ValueError("Missing original Aseprite data/gui.xml, fonts or icons")

    target_data.parent.mkdir(parents=True, exist_ok=True)
    native_out.parent.mkdir(parents=True, exist_ok=True)
    temporary_data = target_data.with_name(target_data.name + ".installing")
    temporary_native = native_out.with_name(native_out.name + ".installing")
    if temporary_data.exists():
        shutil.rmtree(temporary_data)
    if temporary_native.exists():
        temporary_native.unlink()
    try:
        shutil.copytree(original_data, temporary_data, symlinks=False)
        shutil.copy2(args.engine, temporary_native)
        verify_aarch64_shared_lib(temporary_native)
        if target_data.exists():
            shutil.rmtree(target_data)
        temporary_data.replace(target_data)
        temporary_native.replace(native_out)
        # Reuse the original launcher artwork; no replacement application UI.
        icon = destination / "res/drawable/aseprite_icon.png"
        icon.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(original_data / "icons/ase256.png", icon)
    finally:
        if temporary_data.exists():
            shutil.rmtree(temporary_data)
        if temporary_native.exists():
            temporary_native.unlink()
    print(f"Staged ORIGINAL Aseprite assets: {target_data}")
    print(f"Staged linked Android AArch64 library: {native_out}")
    print("Private packaging only; no APK or original engine binary distributed.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)

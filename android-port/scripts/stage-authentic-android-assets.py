#!/usr/bin/env python3
"""Stage UNMODIFIED original Aseprite data and a verified AArch64 engine.

Only stage real source assets and an authentic, already compiled libaseprite.so.
This script does not compile, sign, publish, or distribute Aseprite.
"""
import argparse
from pathlib import Path
import shutil
import struct
import sys

ELF_MAGIC = b"\x7fELF"
EM_AARCH64 = 183

def verify_aarch64_shared_lib(path: Path) -> None:
    with path.open("rb") as f:
        header = f.read(64)
    if len(header) < 64 or header[:4] != ELF_MAGIC:
        raise ValueError(f"{path}: not a genuine ELF shared library")
    if header[4] != 2 or header[5] != 1:
        raise ValueError(f"{path}: expected 64-bit little-endian ELF")
    if struct.unpack_from("<H", header, 16)[0] != 3:
        raise ValueError(f"{path}: ELF is not a shared object (ET_DYN)")
    if struct.unpack_from("<H", header, 18)[0] != EM_AARCH64:
        raise ValueError(f"{path}: expected machine AArch64")
    if path.stat().st_size < 1024:
        raise ValueError(f"{path}: too small to be the real Aseprite engine")

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument("--engine", type=Path, required=True)
    p.add_argument("--destination", type=Path, default=None)
    args = p.parse_args()

    root = args.root.resolve()
    original_data = root / "data"
    assets = args.destination or root / "android-port/app/src/main"
    target_data = assets / "assets/data"
    native_out = assets / "jniLibs/arm64-v8a/libaseprite.so"

    # Validate all inputs BEFORE touching the build destination.
    needed = ["gui.xml", "fonts", "icons"]
    if not original_data.is_dir() or any(not (original_data / name).exists()
                                         for name in needed):
        raise ValueError("Missing original Aseprite data/gui.xml, fonts or icons")
    if not args.engine.is_file():
        raise ValueError("Missing authentic libaseprite.so")
    verify_aarch64_shared_lib(args.engine)

    if target_data.exists():
        shutil.rmtree(target_data)
    shutil.copytree(original_data, target_data, symlinks=False)
    native_out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.engine, native_out)
    print(f"Staged ORIGINAL Aseprite resources: {target_data}")
    print(f"Staged authentic Android AArch64 editor: {native_out}")
    print("NOT published. Package/install only in a private environment.")

if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError) as ex:
        print(f"ERROR: {ex}", file=sys.stderr)
        raise SystemExit(2)

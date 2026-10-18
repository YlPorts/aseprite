#!/usr/bin/env python3
"""Negative checks for Aseprite binary-staging integrity gates."""
import importlib.util
import struct
import tempfile
from pathlib import Path

script = Path(__file__).resolve().parents[1] / "scripts/stage-authentic-android-assets.py"
spec = importlib.util.spec_from_file_location("aseprite_stage_assets", script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def expect_reject(path):
    try:
        module.verify_aarch64_shared_lib(path)
    except (ValueError, OSError):
        return
    raise AssertionError(f"Invalid binary accepted: {path}")

with tempfile.TemporaryDirectory() as scratch:
    path = Path(scratch) / "libaseprite.so"
    path.write_bytes(b"not an Android native shared library")
    expect_reject(path)

    valid_shape = bytearray(1200)
    valid_shape[0:6] = b"\x7fELF\x02\x01"
    struct.pack_into("<H", valid_shape, 16, 3)   # ET_DYN
    struct.pack_into("<H", valid_shape, 18, 62)  # x86_64
    path.write_bytes(valid_shape)
    expect_reject(path)

    struct.pack_into("<H", valid_shape, 18, 183) # AArch64 but no JNI
    path.write_bytes(valid_shape)
    expect_reject(path)

print("PASS: asset staging rejects non-ELF, non-AArch64, missing-editor-JNI binaries")

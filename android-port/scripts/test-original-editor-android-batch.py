#!/usr/bin/env python3
"""Execute the genuine Android engine using QEMU user-mode and SDK Bionic.

This checks native loader/CLI/file formats, not the Android GUI or Activity.
No system stubs, replacement editor or Linux libraries are used by the guest.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def checked_metadata(data, start, size, checksum_start):
    block = bytearray(data[start:start + size])
    expected = bytes(block[checksum_start:checksum_start + 32])
    block[checksum_start:checksum_start + 32] = bytes(32)
    if len(block) != size or hashlib.sha256(block).digest() != expected:
        raise ValueError("Invalid Android logical-partition metadata checksum")


def extract_system(image, output):
    # SDK API 36 images contain GPT -> super -> logical system (ext4).
    # Layouts are defined by AOSP system/core/fs_mgr/liblp/metadata_format.h.
    with image.open("rb") as source:
        source.seek(512)
        gpt = source.read(512)
        if gpt[:8] != b"EFI PART":
            raise ValueError("Expected genuine SDK raw GPT system image")
        table, count, entry_size = struct.unpack_from("<QII", gpt, 72)
        source.seek(table * 512)
        super_base = None
        for _ in range(count):
            entry = source.read(entry_size)
            if entry[56:128].decode("utf-16-le").rstrip("\0") == "super":
                super_base = struct.unpack_from("<Q", entry, 32)[0] * 512
                break
        if super_base is None:
            raise ValueError("Missing SDK super partition")
        source.seek(super_base + 4096)
        geometry = source.read(4096)
        magic, size = struct.unpack_from("<II", geometry)
        if magic != 0x616C4467 or size != 52:
            raise ValueError("Unsupported Android LP geometry")
        checked_metadata(geometry, 0, size, 8)
        source.seek(super_base + 12288)
        header = source.read(256)
        magic, major, minor, size = struct.unpack_from("<IHHI", header)
        if magic != 0x414C5030 or major != 10 or size not in (128, 256):
            raise ValueError("Unsupported Android LP metadata header")
        checked_metadata(header, 0, size, 12)
        source.seek(super_base + 12288 + size)
        tables = source.read(struct.unpack_from("<I", header, 44)[0])
        if hashlib.sha256(tables).digest() != header[48:80]:
            raise ValueError("Invalid Android LP tables checksum")
        part_offset, part_count, part_size = struct.unpack_from("<III", header, 80)
        extent_offset, extent_count, extent_size = struct.unpack_from("<III", header, 92)
        for index in range(part_count):
            name, attrs, first, count, group = struct.unpack_from(
                "<36sIIII", tables, part_offset + index * part_size)
            if name.rstrip(b"\0") != b"system":
                continue
            if first + count > extent_count:
                raise ValueError("Invalid system partition extent count")
            with output.open("wb") as destination:
                for extent in range(first, first + count):
                    sectors, kind, sector, device = struct.unpack_from(
                        "<QIQI", tables, extent_offset + extent * extent_size)
                    if kind != 0 or device != 0:
                        raise ValueError("Unsupported non-linear SDK system extent")
                    source.seek(super_base + sector * 512)
                    remaining = sectors * 512
                    while remaining:
                        chunk = source.read(min(remaining, 1024 * 1024))
                        if not chunk:
                            raise ValueError("Truncated SDK system image")
                        destination.write(chunk)
                        remaining -= len(chunk)
            return
    raise ValueError("Missing logical system partition")


def snapshot(path):
    data = path.read_bytes()
    size, magic, frames, width, height, depth = struct.unpack_from("<I5H", data)
    if magic != 0xA5E0 or size != len(data):
        raise ValueError(f"Invalid native Aseprite document: {path}")
    flags = struct.unpack_from("<I", data, 14)[0]
    layers, timeline = [], []
    offset = 128
    for _ in range(frames):
        length, magic, chunks, duration = struct.unpack_from("<I3H", data, offset)
        if magic != 0xF1FA or offset + length > len(data):
            raise ValueError("Invalid Aseprite frame")
        if chunks == 0xFFFF:
            chunks = struct.unpack_from("<I", data, offset + 12)[0]
        cursor, cels = offset + 16, []
        for _ in range(chunks):
            length_chunk, kind = struct.unpack_from("<IH", data, cursor)
            if length_chunk < 6 or cursor + length_chunk > offset + length:
                raise ValueError("Invalid Aseprite chunk")
            chunk = data[cursor + 6:cursor + length_chunk]
            if kind == 0x2004:
                layer_flags, layer_type, level, _, _, blend = struct.unpack_from("<6H", chunk)
                name_len = struct.unpack_from("<H", chunk, 16)[0]
                layers.append((layer_flags, layer_type, level, blend,
                               chunk[12] if flags & 1 else 255,
                               chunk[18:18 + name_len].decode("utf-8")))
            elif kind == 0x2005:
                layer, x, y, opacity, cel_type, z = struct.unpack_from("<HhhBHh", chunk)
                cels.append((layer, x, y, opacity, 2 if cel_type == 0 else cel_type, z))
            cursor += length_chunk
        timeline.append((duration, cels))
        offset += length
    return (frames, width, height, depth, layers, timeline)


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--engine", type=Path, required=True)
    cli.add_argument("--system-image", type=Path, required=True)
    cli.add_argument("--ndk", type=Path,
                     default=os.environ.get("ANDROID_NDK_HOME"))
    cli.add_argument("--workdir", type=Path, default=ROOT / "build/android-native-batch-test")
    args = cli.parse_args()
    engine, work = args.engine.resolve(), args.workdir.resolve()
    spec = importlib.util.spec_from_file_location(
        "stage_assets", Path(__file__).with_name("stage-authentic-android-assets.py"))
    stage = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(stage)
    stage.verify_aarch64_shared_lib(engine)
    qemu, debugfs = shutil.which("qemu-aarch64"), shutil.which("debugfs")
    if not qemu or not debugfs or not args.ndk:
        raise ValueError("Requires qemu-user, e2fsprogs and ANDROID_NDK_HOME")
    work.mkdir(parents=True, exist_ok=True)
    system = work / "system.img"
    extract_system(args.system_image, system)
    runtime = work / "runtime"
    if runtime.exists():
        shutil.rmtree(runtime)
    (runtime / "system").mkdir(parents=True, exist_ok=True)

    def ext4(image, command):
        result = subprocess.run([debugfs, "-R", command, str(image)],
                                capture_output=True, text=True, check=True)
        # debugfs sometimes returns zero despite a failed extraction.
        if "No such file" in result.stderr:
            raise ValueError(result.stderr)

    ext4(system, f'rdump /system/lib64 "{runtime / "system"}"')
    for name in ("com.android.runtime", "com.android.i18n"):
        apex = work / (name + ".apex")
        payload = work / (name + ".img")
        ext4(system, f'dump /system/apex/{name}.apex "{apex}"')
        with zipfile.ZipFile(apex) as archive, archive.open("apex_payload.img") as source, \
                payload.open("wb") as destination:
            shutil.copyfileobj(source, destination)
        destination = runtime / "apex" / name
        destination.mkdir(parents=True, exist_ok=True)
        ext4(payload, f'rdump / "{destination}"')
    linker = runtime / "apex/com.android.runtime/bin/linker64"
    if not linker.is_file() or not (runtime / "system/lib64/libandroid.so").is_file():
        raise ValueError("Missing genuine Android loader or platform library")
    launcher = work / "original-editor-cli"
    compiler = args.ndk / "toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"
    subprocess.run([str(compiler), str(ROOT / "android-port/tests/original_editor_cli_main.cpp"),
                    "-L" + str(engine.parent), "-laseprite", "-static-libstdc++",
                    "-o", str(launcher)], check=True)
    user = work / "user"
    user.mkdir(exist_ok=True)
    libs = [engine.parent, runtime / "system/lib64",
            runtime / "apex/com.android.runtime/lib64/bionic",
            runtime / "apex/com.android.i18n/lib64"]
    command = [qemu, "-L", str(runtime), "-E", f"HOME={user}",
               "-E", f"ASEPRITE_USER_FOLDER={user}",
               "-E", f"ASEPRITE_ANDROID_DATA_DIR={ROOT / 'data'}",
               "-E", "LD_LIBRARY_PATH=" + ":".join(map(str, libs)), str(linker), str(launcher)]

    def run(label, *options):
        result = subprocess.run(command + list(map(str, options)),
                                capture_output=True, text=True, timeout=60)
        (work / (label + ".log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f"{label}: Android engine exited {result.returncode}\n{result.stderr[-2000:]}")
        return result.stdout

    version = run("version", "--batch", "--version").strip()
    if not version.startswith("Aseprite 1."):
        raise ValueError("Original editor version missing")
    print(version, flush=True)
    for name in ("1empty3", "groups3abc", "tags3x123reps", "2x3tilemap-indexed"):
        source = ROOT / "tests/sprites" / (name + ".aseprite")
        saved = work / (name + ".ase")
        run(name + "-save", "--batch", source, "--save-as", saved)
        if snapshot(source) != snapshot(saved):
            raise ValueError(f"Layers/cels/frames changed after native save: {name}")
        sheets, metadata = [], []
        for label, path in (("before", source), ("reopened", saved)):
            image, data = work / f"{name}-{label}.png", work / f"{name}-{label}.json"
            run(name + "-" + label, "--batch", "--list-layer-hierarchy", "--list-tags",
                "--list-slices", "--filename-format", "{frame}", path,
                "--sheet", image, "--data", data, "--format", "json-array")
            sheets.append(image.read_bytes())
            document = json.loads(data.read_text())
            document["meta"].pop("image", None)
            metadata.append(document)
        if sheets[0] != sheets[1] or metadata[0] != metadata[1]:
            raise ValueError(f"Rendered pixels or export metadata changed: {name}")
        print(f"PASS Android original engine: {name}, layers/frames/cels, .ase save/reopen and spritesheet equivalence", flush=True)
    png = ROOT / "data/icons/ase32.png"
    imported = work / "imported.aseprite"
    run("png-import", "--batch", png, "--save-as", imported)
    for label, path in (("original", png), ("reopened", imported)):
        run("png-" + label, "--batch", path, "--save-as", work / (label + ".png"))
    if (work / "original.png").read_bytes() != (work / "reopened.png").read_bytes():
        raise ValueError("PNG pixels changed through .aseprite save/reopen")
    print("PASS Android original engine: PNG import, .aseprite save/reopen and pixel equivalence", flush=True)
    for extension in ("gif", "jpg", "webp"):
        encoded = work / ("encoded." + extension)
        run(extension + "-encode", "--batch", png, "--save-as", encoded)
        data = work / (extension + "-decoded.json")
        run(extension + "-decode", "--batch", encoded, "--format", "json-array",
            "--data", data, "--sheet", work / (extension + "-decoded.png"))
        frames = json.loads(data.read_text())["frames"]
        if len(frames) != 1 or frames[0]["sourceSize"] != {"w": 32, "h": 32}:
            raise ValueError(f"Unexpected decoded {extension} dimensions/frames")
        print(f"PASS Android original engine: {extension} encode, reopen and dimensions", flush=True)
    print("Native CLI tests only; Android Activity, GUI, touch, IME and Galaxy A15 remain UNTESTED.", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)

#!/usr/bin/env python3
"""Inspect the APK actually produced by Gradle; never a device/runtime test."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import shutil
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "stage_assets", Path(__file__).with_name("stage-authentic-android-assets.py"))
stage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage)


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("apk", type=Path)
    args = cli.parse_args()
    with zipfile.ZipFile(args.apk) as apk:
        names = apk.namelist()
        if len(names) != len(set(names)):
            raise ValueError("Duplicate APK entries")
        expected_engine = "lib/arm64-v8a/libaseprite.so"
        libraries = [name for name in names if name.startswith("lib/") and name.endswith(".so")]
        if libraries != [expected_engine]:
            raise ValueError(f"Unexpected native payload: {libraries}")
        for name in ("classes.dex", "AndroidManifest.xml"):
            if not apk.getinfo(name).file_size:
                raise ValueError(f"Empty {name}")
        with tempfile.TemporaryDirectory(prefix="aseprite-apk-elf-") as temp:
            engine = Path(temp) / "libaseprite.so"
            with apk.open(expected_engine) as source, engine.open("wb") as output:
                shutil.copyfileobj(source, output)
            stage.verify_aarch64_shared_lib(engine)
        critical = (
            "gui.xml", "fonts/fonts.xml", "fonts/aseprite_font.png",
            "icons/ase256.png", "extensions/aseprite-theme/theme.xml",
            "extensions/aseprite-theme/sheet.png",
        )
        for relative in critical:
            original = ROOT / "data" / relative
            if apk.read("assets/data/" + relative) != original.read_bytes():
                raise ValueError(f"Missing or changed original Aseprite resource: {relative}")
        resource_count = sum(name.startswith("assets/data/") for name in names)
    print(f"Verified packaged AArch64 engine, seven defined JNI exports and {resource_count} original resource entries")
    with args.apk.open("rb") as stream:
        print(f"APK SHA-256: {hashlib.file_digest(stream, 'sha256').hexdigest()}")
    print("Packaging verification only. Startup, editing and Android device tests remain pending.")


if __name__ == "__main__":
    main()

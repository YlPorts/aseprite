#!/usr/bin/env python3
"""Test real ELF symbol-table checks with cross-compiled non-editor fixtures."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

script = Path(__file__).resolve().parents[1] / "scripts/stage-authentic-android-assets.py"
spec = importlib.util.spec_from_file_location("aseprite_stage_assets", script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def rejected(path):
    try:
        module.verify_aarch64_shared_lib(path)
    except ValueError:
        return
    raise AssertionError(f"Invalid native library passed validation: {path}")


def compile_fixture(clang, source, dest, triple):
    subprocess.run([clang, "--target=" + triple, "-fuse-ld=lld", "-shared",
                    "-nostdlib", "-fvisibility=default", str(source), "-o", str(dest)],
                   check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    so = root / "libaseprite.so"
    so.write_bytes(b"not ELF")
    rejected(so)

    clang = shutil.which("clang")
    if not clang or not shutil.which("ld.lld"):
        raise RuntimeError("Clang and lld required for real ELF regression fixtures")
    source = root / "fixture.c"
    prefix = module.JNI_PREFIX
    source.write_text("\n".join(
        f'__attribute__((visibility("default"))) void {prefix}{name}(void) {{}}'
        for name in module.REQUIRED_JNI) + "\n")
    compile_fixture(clang, source, so, "aarch64-linux-gnu")
    module.verify_aarch64_shared_lib(so)
    subprocess.run([sys.executable, str(script), "--root", str(root),
                    "--engine", str(so), "--verify-only"], check=True,
                   stdout=subprocess.PIPE)
    assert not (root / "android-port").exists(), "Verification must not stage fixtures"

    absent = root / "without_ime.c"
    absent.write_text("\n".join(
        f'__attribute__((visibility("default"))) void {prefix}{name}(void) {{}}'
        for name in module.REQUIRED_JNI if name != "nativeCommitText") + "\n")
    compile_fixture(clang, absent, so, "aarch64-linux-gnu")
    rejected(so)
    # Symbol name embedded as inert text must never satisfy JNI checks.
    absent.write_text(absent.read_text() +
                      f'const char *unused = "{prefix}nativeCommitText";\n')
    compile_fixture(clang, absent, so, "aarch64-linux-gnu")
    rejected(so)
    compile_fixture(clang, source, so, "x86_64-linux-gnu")
    rejected(so)
print("PASS: genuine ARM64 ELF table verification and rejection of x86, inert strings, missing JNI")

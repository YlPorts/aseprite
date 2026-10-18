#!/usr/bin/env python3
"""Run the Android overlays twice against the ACTUAL checked-out Aseprite+LAF.

Does not alter either Git submodule or user's checkout; works in a tempfile.
This catches changed upstream anchors, non-idempotent overlays, and an
accidental Android fallback to SkiaWindowX11.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

REPO = Path(__file__).resolve().parents[2]
PORT = REPO / "android-port"
SCRIPTS = [
    PORT / "scripts/patch-laf-headless-android.py",
    PORT / "scripts/patch-laf-android-raster.py",
    PORT / "scripts/patch-laf-android-runtime.py",
    PORT / "scripts/patch-aseprite-android-engine.py",
]
ORIGINAL_FILES = [
    "CMakeLists.txt",
    "src/app/resource_finder.cpp",
    "laf/base/CMakeLists.txt",
    "laf/base/platform.h",
    "laf/base/platform_unix.cpp",
    "laf/os/CMakeLists.txt",
    "laf/os/common/event_queue.cpp",
    "laf/os/skia/skia_window.h",
    "laf/os/skia/skia_window.cpp",
    "laf/os/skia/skia_surface.cpp",
    "laf/os/skia/skia_system.h",
    "laf/cmake/FindSkia.cmake",
]

def original_files_are_present():
    absent = [str(REPO / rel) for rel in ORIGINAL_FILES
              if not (REPO / rel).is_file()]
    if absent:
        raise RuntimeError("Original Aseprite submodule missing: " + ", ".join(absent))

def clone_required_sources(destination):
    for relative in ORIGINAL_FILES:
        source = REPO / relative
        output = destination / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, output)
    (destination / "android-port").symlink_to(PORT, target_is_directory=True)

def run_overlays(destination, dry_run=False):
    for script in SCRIPTS:
        argv = [sys.executable, str(script), str(destination)]
        if dry_run:
            argv.append("--dry-run")
        subprocess.run(argv, check=True, text=True)

def snapshot(destination):
    return {str(path.relative_to(destination)): path.read_bytes()
            for path in destination.rglob("*")
            if path.is_file() and "android-port" not in
            path.relative_to(destination).parts}

def verify(source_root):
    cmake = (source_root / "laf/os/CMakeLists.txt").read_text()
    skia = (source_root / "laf/os/skia/skia_system.h").read_text()
    cmake_skia = (source_root / "laf/cmake/FindSkia.cmake").read_text()
    unix = (source_root / "laf/base/platform_unix.cpp").read_text()
    surface = (source_root / "laf/os/skia/skia_surface.cpp").read_text()
    main = (source_root / "laf/os/skia/skia_window.cpp").read_text()
    engine = (source_root / "CMakeLists.txt").read_text()
    source = (source_root / "src/app/resource_finder.cpp").read_text()
    fragment = ("  elseif(ANDROID)\n"
                "    list(APPEND LAF_OS_SOURCES\n"
                "      skia/skia_window_android.cpp)\n"
                "  else()\n"
                "    list(APPEND LAF_OS_SOURCES\n"
                "      skia/skia_window_x11.cpp)")
    assert fragment in cmake, "Android would select Linux/X11 renderer"
    assert "android/window_registry.cpp android/key_state.cpp" in cmake
    assert "SK_SUPPORT_GPU=$<IF:$<BOOL:${ANDROID}>,0,1>" in cmake_skia
    assert "NOT APPLE AND NOT ANDROID AND EXISTS" in cmake_skia
    assert "#include \"base/platform.h\"\n#if LAF_LINUX" in unix
    assert "} // namespace base\n#endif // LAF_LINUX" in unix
    assert "androidUnregisterWindow(nullptr);" in skia
    assert "#if SK_SUPPORT_GPU\n  // The SkSurface_Raster" in surface
    assert "#endif\n\n  return nullptr;" in surface
    assert "Capabilities::MultipleWindows" in skia
    assert "#if LAF_ANDROID" in skia
    assert "androidRegisterWindow(AddRef(this));" in main
    assert "ASEPRITE_ANDROID_GUI" in engine
    assert "ASEPRITE_ANDROID_DATA_DIR" in source
    assert (source_root / "laf/os/android/window.h").is_file()
    assert (source_root / "laf/os/android/system.h").is_file()

def main():
    original_files_are_present()
    with tempfile.TemporaryDirectory(prefix="aseprite-laf-integration-") as temp:
        tmp = Path(temp)
        clone_required_sources(tmp)
        run_overlays(tmp)
        verify(tmp)
        first = snapshot(tmp)
        run_overlays(tmp)
        assert snapshot(tmp) == first, "Repeated overlays changed upstream again"
        run_overlays(tmp, dry_run=True)
        assert snapshot(tmp) == first, "Dry run changed upstream"
    print("PASS: original LAF Android patches, Skia selection, idempotence and dry-run")

if __name__ == "__main__":
    main()

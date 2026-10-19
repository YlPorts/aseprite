#!/usr/bin/env python3
"""Prepare original LAF for a NON-GUI Android library build.

Only uses exact matching CMake/header edits, preserves original submodule
history, and refuses to pretend that SkiaWindowAndroid already exists.
Use in a throwaway working checkout, never in a published binary release.
"""
from pathlib import Path
import argparse
import shutil

def replace_once(path, old, new, dry, accepted_later=None):
    text = path.read_text(encoding="utf-8")
    if new in text or (accepted_later and accepted_later in text):
        print("Already patched:", path)
        return
    if text.count(old) != 1:
        raise RuntimeError(f"{path}: upstream changed: expected one anchor, found {text.count(old)}")
    if not dry:
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print("CHECK" if dry else "PATCH", path)

def copy_checked(source, destination, dry):
    if destination.exists():
        if destination.read_bytes() != source.read_bytes():
            raise RuntimeError(f"{destination} exists with different contents")
        print("Already patched:", destination)
        return
    if not dry:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
    print("CHECK" if dry else "ADD", destination)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("aseprite_root", type=Path)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    root = args.aseprite_root.resolve()
    laf = root / "laf"
    own = Path(__file__).resolve().parents[1]
    if not (laf / "os/CMakeLists.txt").is_file() or not (laf / "base/CMakeLists.txt").is_file():
        parser.error("Run git submodule update --init --recursive in the original source checkout")
    base = laf / "base/CMakeLists.txt"
    replace_once(base,
        "  else()\n    target_compile_definitions(laf-base PUBLIC LAF_LINUX)\n    target_link_libraries(laf-base pthread)\n",
        "  elseif(ANDROID)\n    target_compile_definitions(laf-base PUBLIC LAF_ANDROID)\n    target_link_libraries(laf-base pthread)\n  else()\n    target_compile_definitions(laf-base PUBLIC LAF_LINUX)\n    target_link_libraries(laf-base pthread)\n",
        args.dry_run)
    p = laf / "base/platform.h"
    replace_once(p, "    Linux,\n", "    Linux,\n    Android,\n", args.dry_run)
    replace_once(p,
        "  static constexpr OS os =\n#if LAF_WINDOWS",
        "  static constexpr OS os =\n#if LAF_ANDROID\n    OS::Android\n#elif LAF_WINDOWS",
        args.dry_run)
    # Android also compiles the Unix source list, but this implementation
    # uses get_linux_release_info() and types declared only for LAF_LINUX.
    # Guard the Linux-only function without altering desktop behavior.
    p = laf / "base/platform_unix.cpp"
    replace_once(p,
        '#include "base/platform.h"',
        '#include "base/platform.h"\n#if LAF_LINUX',
        args.dry_run)
    replace_once(p,
        '} // namespace base',
        '} // namespace base\n#endif // LAF_LINUX',
        args.dry_run)
    # C11 aligned_alloc is available in Bionic only from API 28. Keep
    # the port's API 26 minimum using the compatible POSIX allocation/free.
    replace_once(laf / "base/memory.cpp",
        '  return _aligned_malloc(bytes, alignment);\n#else',
        '  return _aligned_malloc(bytes, alignment);\n'
        '#elif LAF_ANDROID\n'
        '  void* mem = nullptr;\n'
        '  if (posix_memalign(&mem, alignment, bytes) != 0)\n'
        '    return nullptr;\n'
        '  return mem;\n#else',
        args.dry_run)
    p = laf / "os/common/event_queue.cpp"
    replace_once(p,
        '#if LAF_WINDOWS\n  #include "os/win/event_queue.h"',
        '#if LAF_ANDROID\n  #include "os/android/event_queue.h"\n#elif LAF_WINDOWS\n  #include "os/win/event_queue.h"',
        args.dry_run)
    p = laf / "os/CMakeLists.txt"
    replace_once(p,
        'if(WIN32)\n  list(APPEND LAF_OS_SOURCES\n    win/color_space.cpp',
        'if(ANDROID)\n  list(APPEND LAF_OS_SOURCES android/event_queue.cpp)\nelseif(WIN32)\n  list(APPEND LAF_OS_SOURCES\n    win/color_space.cpp',
        args.dry_run,
        accepted_later='if(ANDROID)\n  list(APPEND LAF_OS_SOURCES android/event_queue.cpp ')
    replace_once(p,
        'if(LAF_BACKEND STREQUAL "skia")\n  if(APPLE)',
        'if(LAF_BACKEND STREQUAL "skia")\n  if(ANDROID)\n    message(FATAL_ERROR "Android SkiaWindow backend not implemented: do not build a fake editor")\n  endif()\n  if(APPLE)',
        args.dry_run,
        accepted_later='# Android window is selected in the Skia sources dispatch below.')
    replace_once(p,
        "set(LAF_OS_PLATFORM_LIBS)\nif(WIN32)",
        "set(LAF_OS_PLATFORM_LIBS)\nif(ANDROID)\n  find_package(Threads REQUIRED)\n  list(APPEND LAF_OS_PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})\nelseif(WIN32)",
        args.dry_run,
        accepted_later="list(APPEND LAF_OS_PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT} android log)")
    original = own / "platform/laf/os/android"
    for name in ("event_queue.h", "event_queue.cpp"):
        copy_checked(original / name, laf / "os/android" / name, args.dry_run)
    copy_checked(own / "src/android/event_queue_core.h",
                 laf / "os/android/event_queue_core.h", args.dry_run)
    print("Headless LAF Android queue prepared; real Android System/Window/Skia still required.")

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Configure the Android JNI CMake target in isolation, with stub sources.

This catches CMake topology mistakes. It NEVER links or packages fake editor
binaries; the full actual Aseprite engine still needs its own NDK CI build.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    root = Path(__file__).resolve().parents[2]
    cmake_source = root / 'android-port/native-app/CMakeLists.txt'
    if not cmake_source.is_file():
        raise RuntimeError(f'Need genuine port source at {cmake_source}')
    with tempfile.TemporaryDirectory(prefix='aseprite-cmake-contract-') as temp:
        source = Path(temp) / 'source'
        build = Path(temp) / 'build'
        script = source / 'android-port/native-app/CMakeLists.txt'
        script.parent.mkdir(parents=True)
        shutil.copy2(cmake_source, script)
        files = (
            'src/main/main.cpp',
            'android-port/src/android/jni_editor_entry.cpp',
            'android-port/src/android/laf_event_bridge.cpp',
            'android-port/src/android/touch_router.cpp',
            'android-port/src/android/native_motion.cpp',
        )
        for f in files:
            target = source / f
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text('// CMake configure-only fixture: no editor code\n')
        (source / 'CMakeLists.txt').write_text('''\
cmake_minimum_required(VERSION 3.20)
project(original_aseprite LANGUAGES CXX)
set(ANDROID TRUE)
set(LAF_BACKEND skia)
set(ENABLE_ASEPRITE_EXE OFF)
add_library(app-lib INTERFACE)
add_library(laf-os INTERFACE)
add_custom_target(copy_data)
add_subdirectory(android-port/native-app)
if(NOT TARGET aseprite OR NOT TARGET aseprite-android-touch)
  message(FATAL_ERROR "Original JNI/touch build targets missing")
endif()
if(NOT PROJECT_NAME STREQUAL "original_aseprite")
  message(FATAL_ERROR "Nested Android port overwrote original CMake project")
endif()
''')
        command = ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja']
        subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        output = (build / 'build.ninja').read_text()
        if 'libaseprite.so' not in output:
            raise RuntimeError('Original shared JNI target was not generated')
        if 'aseprite-android-touch' not in output:
            raise RuntimeError('Android native touch target was not generated')
        # Removing original laf-os must fail closed; no replacement GUI.
        original_cmake = (source / 'CMakeLists.txt').read_text()
        (source / 'CMakeLists.txt').write_text(
            original_cmake.replace('add_library(laf-os INTERFACE)\n', ''))
        broken = subprocess.run(
            ['cmake', '-S', str(source), '-B', str(Path(temp) / 'missing-laf'), '-G', 'Ninja'],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if broken.returncode == 0 or 'Missing original Aseprite targets' not in broken.stdout:
            raise RuntimeError('Missing original laf-os was incorrectly accepted')
    print('PASS: original CMake project preserved; real JNI/touch targets configured (no build)')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        raise SystemExit(1)

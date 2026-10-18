#!/usr/bin/env python3
"""Follow-up overlay after patch-laf-headless-android.py and patch-laf-android-raster.py."""
import argparse
from pathlib import Path
import shutil

def update(path,old,new,dry):
    data=path.read_text(encoding='utf-8')
    if new in data: return
    if data.count(old)!=1:
        raise RuntimeError(f'{path}: upstream mismatch on runtime overlay: {data.count(old)}')
    if not dry: path.write_text(data.replace(old,new,1),encoding='utf-8')

def main():
    p=argparse.ArgumentParser()
    p.add_argument('aseprite_root',type=Path)
    p.add_argument('--dry-run',action='store_true')
    a=p.parse_args()
    root=a.aseprite_root.resolve()
    laf=root/'laf'
    port=root/'android-port/platform/laf'
    cmake=laf/'os/CMakeLists.txt'
    if not cmake.exists() or not port.exists(): p.error('Missing source tree and overlay')
    update(cmake,'android/event_queue.cpp android/window.cpp android/screen.cpp)',
        'android/event_queue.cpp android/window.cpp android/screen.cpp '
        'android/window_registry.cpp android/key_state.cpp)',a.dry_run)
    for relative in ('os/android/window_registry.h','os/android/window_registry.cpp',
                     'os/android/key_state.h','os/android/key_state.cpp'):
        source=port/relative
        destination=laf/relative
        if destination.exists():
            if destination.read_bytes()!=source.read_bytes():
                raise RuntimeError(f'{destination}: differs from Android overlay')
        elif not a.dry_run:
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source,destination)
    system=laf/'os/android/system.h'
    update(system,'#include "os/android/screen.h"',
        '#include "os/android/screen.h"\n#include "os/android/key_state.h"',a.dry_run)
    update(system,'public:\n  ScreenRef primaryScreen() override',
        'public:\n  bool isKeyPressed(KeyScancode key) override { return androidIsKeyPressed(key); }\n'
        '  ScreenRef primaryScreen() override',a.dry_run)
    # The registered window is intentionally held until the app_main thread
    # exits and JNI releases it; no unprotected cross-thread raw pointer.
    skia=laf/'os/skia/skia_window.cpp'
    update(skia,'#include "os/skia/skia_window.h"',
        '#include "os/skia/skia_window.h"\n#if LAF_ANDROID\n'
        '  #include "os/android/window_registry.h"\n#endif',a.dry_run)
    update(skia,'  initializeSurface();\n}',
        '  initializeSurface();\n#if LAF_ANDROID\n'
        '  androidRegisterWindow(AddRef(this));\n#endif\n}',a.dry_run)
    print('Android LAF runtime overlay prepared. Full Skia/Activity compilation still required.')
if __name__=='__main__':
    main()

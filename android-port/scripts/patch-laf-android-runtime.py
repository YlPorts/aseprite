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
    # The registered window is intentionally held until the app_main thread
    # exits and JNI releases it; no unprotected cross-thread raw pointer.
    skia=laf/'os/skia/skia_window.cpp'
    update(skia,'#include "os/skia/skia_window.h"',
        '#include "os/skia/skia_window.h"\n#if LAF_ANDROID\n'
        '  #include "os/android/window_registry.h"\n#endif',a.dry_run)
    update(skia,'  initializeSurface();\n}',
        '  initializeSurface();\n#if LAF_ANDROID\n'
        '  androidRegisterWindow(AddRef(this));\n#endif\n}',a.dry_run)
    # Release the strong JNI window registry before CommonSystem clears events
    # and destroys the original Skia resources.
    skiasys=laf/'os/skia/skia_system.h'
    update(skiasys,
        '#include "os/skia/skia_window.h"',
        '#include "os/skia/skia_window.h"\n'
        '#if LAF_ANDROID\n  #include "os/android/window_registry.h"\n#endif',
        a.dry_run)
    update(skiasys,
        '~SkiaSystem() { destroyInstance(); }',
        '~SkiaSystem() {\n'
        '#if LAF_ANDROID\n'
        '    androidUnregisterWindow(nullptr);\n'
        '#endif\n'
        '    destroyInstance();\n'
        '  }',
        a.dry_run)
    update(skiasys,
        '    return Capabilities(int(Capabilities::MultipleWindows) | int(Capabilities::CanResizeWindow) |',
        '#if LAF_ANDROID\n'
        '    // One Android SurfaceView; do not advertise unsupported native windows.\n'
        '    return Capabilities(int(Capabilities::WindowScale) | int(Capabilities::ColorSpaces));\n'
        '#else\n'
        '    return Capabilities(int(Capabilities::MultipleWindows) | int(Capabilities::CanResizeWindow) |',
        a.dry_run)
    update(skiasys,
        '    );\n  }\n\n  void setTabletOptions(',
        '    );\n#endif\n  }\n\n  void setTabletOptions(',
        a.dry_run)
    # RefEmpty() cannot supply an Android system/default typeface. Use the
    # native m124 manager already built by target_os="android" instead.
    fonts=laf/'text/skia_font_mgr.cpp'
    update(fonts,
        '#elif LAF_LINUX\n  #include "include/ports/SkFontMgr_fontconfig.h"',
        '#elif LAF_ANDROID\n  #include "include/ports/SkFontMgr_android.h"\n'
        '#elif LAF_LINUX\n  #include "include/ports/SkFontMgr_fontconfig.h"',
        a.dry_run)
    update(fonts,
        '#elif LAF_LINUX\n  m_skFontMgr = SkFontMgr_New_FontConfig(nullptr);',
        '#elif LAF_ANDROID\n  m_skFontMgr = SkFontMgr_New_Android(nullptr);\n'
        '#elif LAF_LINUX\n  m_skFontMgr = SkFontMgr_New_FontConfig(nullptr);',
        a.dry_run)
    print('Android LAF runtime overlay prepared. Full Skia/Activity compilation still required.')
if __name__=='__main__':
    main()

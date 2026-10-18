#!/usr/bin/env python3
"""Experimental raster Android window overlay for original LAF.

Run AFTER patch-laf-headless-android.py in a full Aseprite checkout.
No Activity or installable APK is provided. Upstream mismatches fail closed.
"""
import argparse
from pathlib import Path
import shutil

def replace_once(path, old, new, dry, accepted_later=None):
    value=path.read_text(encoding='utf-8')
    if new in value or (accepted_later and accepted_later in value):
        print('EXISTS',path)
        return
    if value.count(old)!=1:
        raise RuntimeError(f'{path}: upstream anchor mismatch: {value.count(old)}')
    if not dry:
        path.write_text(value.replace(old,new,1),encoding='utf-8')
    print('CHECK' if dry else 'PATCH',path)

def copy(src,dst,dry):
    if dst.exists():
        if src.read_bytes()!=dst.read_bytes():
            raise RuntimeError(f'{dst}: differing existing file')
    else:
        if not dry:
            dst.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(src,dst)
        print('CHECK' if dry else 'COPY',dst)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('aseprite_root',type=Path)
    parser.add_argument('--dry-run',action='store_true')
    args=parser.parse_args()
    laf=args.aseprite_root.resolve()/'laf'
    port=Path(__file__).resolve().parents[1]
    if not (laf/'os/skia/skia_system.h').is_file():
        parser.error('Missing original laf git submodule')
    cmake=laf/'os/CMakeLists.txt'
    replace_once(cmake,
        '  list(APPEND LAF_OS_SOURCES android/event_queue.cpp)',
        '  list(APPEND LAF_OS_SOURCES android/event_queue.cpp android/window.cpp android/screen.cpp)',
        args.dry_run,
        accepted_later='android/event_queue.cpp android/window.cpp android/screen.cpp android/window_registry.cpp')
    replace_once(cmake,
        '    message(FATAL_ERROR "Android SkiaWindow backend not implemented: do not build a fake editor")',
        '    # Android window is selected in the Skia sources dispatch below.',
        args.dry_run)
    replace_once(cmake,
        '  elseif(APPLE)\n    list(APPEND LAF_OS_SOURCES\n      skia/skia_window_osx.mm)\n  else()\n    list(APPEND LAF_OS_SOURCES\n      skia/skia_window_x11.cpp)',
        '  elseif(APPLE)\n    list(APPEND LAF_OS_SOURCES\n      skia/skia_window_osx.mm)\n  elseif(ANDROID)\n    list(APPEND LAF_OS_SOURCES\n      skia/skia_window_android.cpp)\n  else()\n    list(APPEND LAF_OS_SOURCES\n      skia/skia_window_x11.cpp)',
        args.dry_run)
    replace_once(cmake,
        '  list(APPEND LAF_OS_PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})\nelseif(WIN32)',
        '  list(APPEND LAF_OS_PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT} android log)\nelseif(WIN32)',
        args.dry_run)
    replace_once(cmake,
        'target_link_libraries(laf-os laf-gfx laf-base)',
        'target_link_libraries(laf-os laf-gfx laf-base)\nif(ANDROID)\n'
        '  target_include_directories(laf-os PRIVATE ${CMAKE_SOURCE_DIR}/android-port/src/android)\n'
        '  target_sources(laf-os PRIVATE\n'
        '    ${CMAKE_SOURCE_DIR}/android-port/src/android/raster_presenter.cpp\n'
        '    ${CMAKE_SOURCE_DIR}/android-port/src/android/android_raster_window.cpp)\n'
        'endif()',args.dry_run)
    win=laf/'os/skia/skia_window.h'
    replace_once(win,
        '#if LAF_WINDOWS\n  #include "os/skia/skia_window_win.h"',
        '#if LAF_ANDROID\n  #include "os/skia/skia_window_android.h"\n'
        'namespace os { using SkiaWindowPlatform = SkiaWindowAndroid; }\n'
        '#elif LAF_WINDOWS\n  #include "os/skia/skia_window_win.h"',
        args.dry_run)
    system=laf/'os/skia/skia_system.h'
    replace_once(system,
        '#if LAF_WINDOWS\n  #include "os/win/color_space.h"',
        '#if LAF_ANDROID\n  #include "os/android/system.h"\n'
        '  #define SkiaSystemBase SystemAndroid\n'
        '#elif LAF_WINDOWS\n  #include "os/win/color_space.h"',
        args.dry_run)
    skia=laf/'cmake/FindSkia.cmake'
    replace_once(skia,
        'elseif(APPLE)\n  find_library(SKIA_OPENGL_LIBRARY OpenGL NAMES GL)\nelse()',
        'elseif(APPLE)\n  find_library(SKIA_OPENGL_LIBRARY OpenGL NAMES GL)\n'
        'elseif(ANDROID)\n  set(SKIA_OPENGL_LIBRARY "")\nelse()',
        args.dry_run)
    replace_once(skia,
        '  SK_SUPPORT_GPU=1\n  SK_ENABLE_SKSL=1\n  SK_GL=1',
        '  SK_SUPPORT_GPU=$<IF:$<BOOL:${ANDROID}>,0,1>\n'
        '  SK_ENABLE_SKSL=1\n  SK_GL=$<IF:$<BOOL:${ANDROID}>,0,1>',
        args.dry_run)
    replace_once(skia,
        'elseif(APPLE)\n  target_compile_definitions(skia INTERFACE\n    SK_BUILD_FOR_MAC)\nelse()',
        'elseif(APPLE)\n  target_compile_definitions(skia INTERFACE\n    SK_BUILD_FOR_MAC)\n'
        'elseif(ANDROID)\n  target_compile_definitions(skia INTERFACE SK_BUILD_FOR_ANDROID)\nelse()',
        args.dry_run)
    replace_once(skia,
        'if(UNIX AND NOT APPLE)\n  # Change the kN32_SkColorType',
        'if(UNIX AND NOT APPLE AND NOT ANDROID)\n  # Change the kN32_SkColorType',
        args.dry_run)
    # Original LAF SkiaSurface::getBitmap() references Skia's GPU-only
    # private raster classes even when SK_SUPPORT_GPU=0. Guard the fallback:
    # Android's software bitmap path returns m_bitmap before this section.
    surface=laf/'os/skia/skia_surface.cpp'
    replace_once(surface,
        '  // The SkSurface_Raster::skBitmap() member function was added by us',
        '#if SK_SUPPORT_GPU\\n'
        '  // The SkSurface_Raster::skBitmap() member function was added by us',
        args.dry_run)
    replace_once(surface,
        '      return &raster->skBitmap();\\n'
        '    }\\n'
        '  }\\n\\n'
        '  return nullptr;',
        '      return &raster->skBitmap();\\n'
        '    }\\n'
        '  }\\n'
        '#endif\\n\\n'
        '  return nullptr;',
        args.dry_run)
    # Do not run the desktop-only libc++ probing logic for Android NDK.
    replace_once(skia,
        'if(UNIX AND NOT APPLE AND EXISTS "${SKIA_LIBRARY_DIR}/args.gn")',
        'if(UNIX AND NOT APPLE AND NOT ANDROID AND EXISTS "${SKIA_LIBRARY_DIR}/args.gn")',
        args.dry_run)
    for rel in ('os/android/screen.h','os/android/screen.cpp',
                'os/android/window.h','os/android/window.cpp',
                'os/android/system.h',
                'os/skia/skia_window_android.h',
                'os/skia/skia_window_android.cpp'):
        copy(port/'platform/laf'/rel,laf/rel,args.dry_run)
    print('Raster Android LAF overlay prepared; Activity, input loop, APK still missing.')

if __name__=='__main__':
    main()

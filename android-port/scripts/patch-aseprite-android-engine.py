#!/usr/bin/env python3
"""Idempotent Android engine-target patch in the ORIGINAL Aseprite checkout."""
import argparse
from pathlib import Path
def replace_once(path,before,after,dry):
    text=path.read_text(encoding='utf-8')
    if after in text:
        print('ALREADY',path)
        return
    if text.count(before)!=1:
        raise RuntimeError(f'Upstream incompatible: {path}: anchor found {text.count(before)} times')
    if not dry:
        path.write_text(text.replace(before,after,1),encoding='utf-8')
    print('CHECK' if dry else 'PATCH',path)
def main():
    cli=argparse.ArgumentParser()
    cli.add_argument('aseprite_root',type=Path)
    cli.add_argument('--dry-run',action='store_true')
    opts=cli.parse_args()
    root=opts.aseprite_root.resolve()
    if not (root/'src/app/resource_finder.cpp').exists() or not (root/'android-port/native-app/CMakeLists.txt').exists():
        cli.error('Missing original Aseprite checkout or android-port/native-app')
    replace_once(root/'CMakeLists.txt','add_subdirectory(src)\n',
        'add_subdirectory(src)\nif(ANDROID AND ASEPRITE_ANDROID_GUI)\n'
        '  add_subdirectory(android-port/native-app)\nendif()\n',opts.dry_run)
    replace_once(root/'src/app/resource_finder.cpp',
        'void ResourceFinder::includeDataDir(const char* filename)\n{\n  char buf[4096];',
        'void ResourceFinder::includeDataDir(const char* filename)\n{\n'
        '#if LAF_ANDROID\n'
        '  if (const char* assets=std::getenv("ASEPRITE_ANDROID_DATA_DIR"))\n'
        '    addPath(base::join_path(assets,filename));\n'
        '#endif\n  char buf[4096];',opts.dry_run)
    replace_once(root/'src/updater/user_agent.cpp',
        '#else\n\n  // ----------------------------------------------------------------------\n  // Unix like',
        '#elif LAF_ANDROID\n\n  os << "Android";\n\n'
        '#else\n\n  // ----------------------------------------------------------------------\n  // Unix like',
        opts.dry_run)
    print('Original Aseprite Android entry target prepared; no APK generated')
if __name__=='__main__':
    main()

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
    replace_once(root/'src/app/fonts/font_path_unix.cpp',
        '  q.push("~/.fonts");\n  q.push("/usr/local/share/fonts");\n  q.push("/usr/share/fonts");',
        '#if LAF_ANDROID\n'
        '  q.push("/system/fonts");\n'
        '  q.push("/product/fonts");\n'
        '  q.push("/system_ext/fonts");\n'
        '  q.push("/vendor/fonts");\n'
        '#else\n'
        '  q.push("~/.fonts");\n  q.push("/usr/local/share/fonts");\n  q.push("/usr/share/fonts");\n'
        '#endif',opts.dry_run)
    # Reflow the genuine tool options instead of clipping the desktop HBox
    # on a narrow portrait surface. Non-Android builds retain their layout.
    context=root/'src/app/ui/context_bar.cpp'
    replace_once(context,'#include "ui/size_hint_event.h"',
        '#include "ui/size_hint_event.h"\n'
        '#if LAF_ANDROID\n#include "ui/resize_event.h"\n#include "mobile_layout.h"\n#endif',opts.dry_run)
    replace_once(root/'src/app/ui/context_bar.h',
        '  void onSizeHint(ui::SizeHintEvent& ev) override;',
        '  void onSizeHint(ui::SizeHintEvent& ev) override;\n'
        '#if LAF_ANDROID\n'
        '  void onResize(ui::ResizeEvent& ev) override;\n'
        '  bool m_androidRelayout = false;\n'
        '#endif',opts.dry_run)
    replace_once(context,
        '  ev.setSizeHint(gfx::Size(0, theme->dimensions.contextBarHeight()));',
        '#if LAF_ANDROID\n'
        '  std::vector<aseandroid::FlowSize> sizes;\n'
        '  for (auto* child : children()) {\n'
        '    if (child->hasFlags(HIDDEN) || child->isDecorative()) continue;\n'
        '    const auto size = child->sizeHint();\n'
        '    sizes.push_back({size.w, size.h});\n'
        '  }\n'
        '  const int width = ev.fitInWidth() > 0 ?\n'
        '    std::max(1, ev.fitInWidth()-border().width()) : 0;\n'
        '  const auto rows = aseandroid::flowRows(sizes, width, childSpacing());\n'
        '  ev.setSizeHint(gfx::Size(0, std::max(theme->dimensions.contextBarHeight(),\n'
        '                                   rows.height+border().height())));\n'
        '#else\n'
        '  ev.setSizeHint(gfx::Size(0, theme->dimensions.contextBarHeight()));\n'
        '#endif',opts.dry_run)
    replace_once(context,'void ContextBar::onToolSetOpacity(const int& newOpacity)',
        '#if LAF_ANDROID\n'
        'void ContextBar::onResize(ui::ResizeEvent& ev)\n'
        '{\n'
        '  setBoundsQuietly(ev.bounds());\n'
        '  const auto area = childrenBounds();\n'
        '  std::vector<ui::Widget*> widgets;\n'
        '  std::vector<aseandroid::FlowSize> sizes;\n'
        '  for (auto* child : children()) {\n'
        '    if (child->hasFlags(HIDDEN) || child->isDecorative()) continue;\n'
        '    widgets.push_back(child);\n'
        '    const auto size = child->sizeHint();\n'
        '    sizes.push_back({size.w, size.h});\n'
        '  }\n'
        '  const auto rows = aseandroid::flowRows(sizes, std::max(1, area.w), childSpacing());\n'
        '  const int neededHeight = std::max(SkinTheme::get(this)->dimensions.contextBarHeight(),\n'
        '                                    rows.height+border().height());\n'
        '  if (neededHeight != bounds().h && parent() && !m_androidRelayout) {\n'
        '    m_androidRelayout = true;\n'
        '    parent()->layout();\n'
        '    m_androidRelayout = false;\n'
        '    return;\n'
        '  }\n'
        '  for (size_t i=0; i<widgets.size(); ++i) {\n'
        '    const auto& cell = rows.cells[i];\n'
        '    widgets[i]->setBounds(gfx::Rect(area.x+cell.x, area.y+cell.y,\n'
        '                                   cell.width, cell.height));\n'
        '  }\n'
        '}\n'
        '#endif\n\n'
        'void ContextBar::onToolSetOpacity(const int& newOpacity)',opts.dry_run)
    main_window=root/'src/app/ui/main_window.cpp'
    for side in ('RIGHT','LEFT'):
        replace_once(main_window,
            '  m_customizableDock->center()->dock(ui::TOP, m_contextBar.get());\n'
            f'  m_customizableDock->center()->dock(ui::{side}, m_toolBar.get());',
            '#if LAF_ANDROID\n'
            '  m_customizableDock->dock(ui::TOP, m_contextBar.get());\n'
            '#else\n'
            '  m_customizableDock->center()->dock(ui::TOP, m_contextBar.get());\n'
            '#endif\n'
            f'  m_customizableDock->center()->dock(ui::{side}, m_toolBar.get());',opts.dry_run)
    manager=root/'src/ui/manager.cpp'
    replace_once(manager,'#include "ui/ui.h"',
        '#include "ui/ui.h"\n#if LAF_ANDROID\n#include "ui/scroll_window.h"\n#endif',opts.dry_run)
    replace_once(manager,'// The redraw state is used to avoid drawing the manager',
        '#if LAF_ANDROID\n'
        '// Keep original dialogs reachable after rotation or opening the IME.\n'
        '// Reuse the original View/scrollbars and never wrap a View twice.\n'
        'void fit_android_window(Window* window, const gfx::Rect& area, gfx::Rect& bounds)\n'
        '{\n'
        '  if (window->isDesktop() || area.isEmpty()) return;\n'
        '  if (bounds.w > area.w || bounds.h > area.h) {\n'
        '    Widget* content = nullptr;\n'
        '    int count = 0;\n'
        '    for (auto* child : window->children()) {\n'
        '      if (child->isDecorative()) continue;\n'
        '      content = child;\n'
        '      ++count;\n'
        '    }\n'
        '    bounds.x = area.x;\n'
        '    bounds.y = area.y;\n'
        '    if (count == 1 && content->type() != kViewWidget)\n'
        '      add_scrollbars(window, area, bounds, AddScrollBarsOption::IfNeeded);\n'
        '    bounds.w = std::min(bounds.w, area.w);\n'
        '    bounds.h = std::min(bounds.h, area.h);\n'
        '  }\n'
        '  bounds.x = std::clamp(bounds.x, area.x, area.x2()-bounds.w);\n'
        '  bounds.y = std::clamp(bounds.y, area.y, area.y2()-bounds.h);\n'
        '}\n'
        '#endif\n\n'
        '// The redraw state is used to avoid drawing the manager',opts.dry_run)
    replace_once(manager,'  // Add the window to manager.\n  insertChild(0, window);',
        '#if LAF_ANDROID\n'
        '  if (!window->isDesktop()) {\n'
        '    auto bounds = window->bounds();\n'
        '    fit_android_window(window, gfx::Rect(gfx::Point(0, 0), parentDisplay->size()), bounds);\n'
        '    window->setBounds(bounds);\n'
        '  }\n'
        '#endif\n\n'
        '  // Add the window to manager.\n  insertChild(0, window);',opts.dry_run)
    replace_once(manager,
        '    window->setBounds(bounds);\n  }\n}\n\nvoid Manager::onBroadcastMouseMessage',
        '#if LAF_ANDROID\n'
        '    fit_android_window(window, new_pos, bounds);\n'
        '#endif\n'
        '    window->setBounds(bounds);\n  }\n}\n\nvoid Manager::onBroadcastMouseMessage',opts.dry_run)
    print('Original Aseprite Android entry target prepared; no APK generated')
if __name__=='__main__':
    main()

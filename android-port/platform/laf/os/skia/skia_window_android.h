#pragma once
#include "os/android/window.h"
#include "os/skia/skia_window_base.h"
namespace os {
class SkiaWindowAndroid : public SkiaWindowBase<WindowAndroid> {
public:
  explicit SkiaWindowAndroid(const WindowSpec& spec);
  // Present the genuine original SkiaSurface raster bitmap to ANativeWindow.
  void swapBuffers() override;
protected:
  void onPaint(const gfx::Rect& rect) override;
};
}

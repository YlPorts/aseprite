#include "os/skia/skia_window_android.h"
#include "os/skia/skia_surface.h"
#include "android_raster_window.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColorType.h"
#include <cstdint>
#include <limits>
namespace os {
SkiaWindowAndroid::SkiaWindowAndroid(const WindowSpec& spec): Base(spec) {
  initColorSpace();
}
void SkiaWindowAndroid::swapBuffers() {
  Base::swapBuffers();
  if (isVisible() && isInitialized()) onPaint(bounds());
}
void SkiaWindowAndroid::onPaint(const gfx::Rect& rect) {
  (void)rect;
  if (!isInitialized() || !surface()) return;
  auto* raster=static_cast<SkiaSurface*>(surface());
  const SkBitmap& bitmap=raster->bitmap();
  if (!bitmap.getPixels()) return;
  aseandroid::PixelOrder order;
  if (bitmap.colorType()==kRGBA_8888_SkColorType)
    order=aseandroid::PixelOrder::RGBA;
  else if (bitmap.colorType()==kBGRA_8888_SkColorType)
    order=aseandroid::PixelOrder::BGRA;
  else return;
  if (bitmap.rowBytes() > static_cast<size_t>(std::numeric_limits<int>::max()))
    return;
  const aseandroid::ReadPixels src{
    static_cast<const uint8_t*>(bitmap.getPixels()),
    bitmap.width(),bitmap.height(),static_cast<int>(bitmap.rowBytes()),order};
  aseandroid::globalRasterWindow().present(src,scale());
}
} // namespace os

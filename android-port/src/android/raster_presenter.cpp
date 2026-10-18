#include "raster_presenter.h"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace aseandroid {
bool blitNearest(const ReadPixels& src, const WritePixels& dst,
                 int scale, DirtyRect dirty) {
  if (!src.bytes || !dst.bytes || src.width < 1 || src.height < 1 ||
      dst.width < 1 || dst.height < 1 || scale < 1 || scale > 16 ||
      dirty.width <= 0 || dirty.height <= 0 ||
      src.width > std::numeric_limits<int>::max()/4 ||
      dst.width > std::numeric_limits<int>::max()/4 ||
      src.rowBytes < src.width*4 || dst.rowBytes < dst.width*4)
    return false;
  const int64_t left = std::max<int64_t>(0, dirty.x);
  const int64_t top = std::max<int64_t>(0, dirty.y);
  const int64_t right = std::min<int64_t>(dst.width, int64_t(dirty.x)+dirty.width);
  const int64_t bottom = std::min<int64_t>(dst.height, int64_t(dirty.y)+dirty.height);
  if (left >= right || top >= bottom) return true;

  // Never smear the outermost GUI pixels across an ultrawide surface.
  // If the window dimensions aren't exact multiples of the UI scale,
  // preserve integer pixels and use opaque neutral bars for the remainder.
  const int64_t scaledW = int64_t(src.width) * scale;
  const int64_t scaledH = int64_t(src.height) * scale;
  const int64_t offsetX = std::max<int64_t>(0, (int64_t(dst.width)-scaledW)/2);
  const int64_t offsetY = std::max<int64_t>(0, (int64_t(dst.height)-scaledH)/2);
  const bool bgra = (src.order == PixelOrder::BGRA);
  for (int64_t y=top; y<bottom; ++y) {
    auto* out = dst.bytes + y*dst.rowBytes;
    for (int64_t x=left; x<right; ++x) {
      auto* target = out + x*4;
      const int64_t sx = x-offsetX;
      const int64_t sy = y-offsetY;
      if (sx < 0 || sy < 0 || sx >= scaledW || sy >= scaledH) {
        target[0] = target[1] = target[2] = 0;
        target[3] = 255;
        continue;
      }
      const auto* px = src.bytes + (sy/scale)*src.rowBytes + (sx/scale)*4;
      target[0] = px[bgra ? 2 : 0];
      target[1] = px[1];
      target[2] = px[bgra ? 0 : 2];
      target[3] = px[3];
    }
  }
  return true;
}
} // namespace aseandroid

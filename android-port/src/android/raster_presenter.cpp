#include "raster_presenter.h"
#include <algorithm>
#include <cstdint>
#include <limits>
namespace aseandroid {
bool blitNearest(const ReadPixels& src, const WritePixels& dst,
                 int scale, DirtyRect dirty) {
  if (!src.bytes || !dst.bytes || src.width<1 || src.height<1 ||
      dst.width<1 || dst.height<1 || scale<1 || scale>16 ||
      dirty.width<=0 || dirty.height<=0 ||
      src.width>std::numeric_limits<int>::max()/4 ||
      dst.width>std::numeric_limits<int>::max()/4 ||
      src.rowBytes<src.width*4 || dst.rowBytes<dst.width*4)
    return false;
  const int64_t left=std::max<int64_t>(0,dirty.x);
  const int64_t top=std::max<int64_t>(0,dirty.y);
  const int64_t right=std::min<int64_t>(dst.width,int64_t(dirty.x)+dirty.width);
  const int64_t bottom=std::min<int64_t>(dst.height,int64_t(dirty.y)+dirty.height);
  if (left>=right || top>=bottom) return true;
  for (int64_t y=top; y<bottom; ++y) {
    auto* out=dst.bytes+y*dst.rowBytes;
    const auto* in=src.bytes+
      std::min<int64_t>(src.height-1,y/scale)*src.rowBytes;
    for (int64_t x=left; x<right; ++x) {
      const auto* pixel=in+std::min<int64_t>(src.width-1,x/scale)*4;
      auto* dest=out+x*4;
      const bool bgra=(src.order==PixelOrder::BGRA);
      dest[0]=pixel[bgra?2:0];
      dest[1]=pixel[1];
      dest[2]=pixel[bgra?0:2];
      dest[3]=pixel[3];
    }
  }
  return true;
}
}
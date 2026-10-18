#pragma once
#include <cstdint>
namespace aseandroid {
enum class PixelOrder { RGBA, BGRA };
struct ReadPixels {
  const uint8_t* bytes=nullptr;
  int width=0,height=0,rowBytes=0;
  PixelOrder order=PixelOrder::RGBA;
};
struct WritePixels { uint8_t* bytes=nullptr; int width=0,height=0,rowBytes=0; };
struct DirtyRect { int x=0,y=0,width=0,height=0; };
// Copy genuine Skia raster pixels into Android RGBA_8888 with sharp
// integer nearest-neighbor scaling and clipped dirty rectangle.
bool blitNearest(const ReadPixels& src,const WritePixels& dst,int scale,DirtyRect dirty);
}
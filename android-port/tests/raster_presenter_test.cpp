#include "raster_presenter.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace aseandroid;
int main() {
  uint8_t src[]={3,2,1,255,6,5,4,255,0xee,0xee,0xee,0xee,
                 9,8,7,255,12,11,10,255,0xee,0xee,0xee,0xee};
  std::vector<uint8_t> dst(6*4*4,0xab);
  ReadPixels a{src,2,2,12,PixelOrder::BGRA};
  WritePixels b{dst.data(),4,4,24};
  assert(blitNearest(a,b,2,{0,0,4,4}));
  auto check=[&](int x,int y,int r,int g,int bl) {
    const int off=y*24+x*4;
    assert(dst[off]==r && dst[off+1]==g && dst[off+2]==bl && dst[off+3]==255);
  };
  check(0,0,1,2,3); check(1,1,1,2,3);
  check(2,0,4,5,6); check(3,1,4,5,6);
  check(0,2,7,8,9); check(3,3,10,11,12);
  std::fill(dst.begin(),dst.end(),0xab);
  assert(blitNearest(a,b,2,{-1,-1,2,2}));
  check(0,0,1,2,3);
  assert(dst[4]==0xab && dst[24]==0xab);
  assert(!blitNearest({src,2,2,4,PixelOrder::BGRA},b,2,{0,0,4,4}));
  assert(!blitNearest(a,b,0,{0,0,4,4}));
  assert(!blitNearest(a,{nullptr,4,4,24},2,{0,0,4,4}));
  std::cout << "PASS: nearest neighbor, BGRA to RGBA, padding, clipping, validation\n";
}

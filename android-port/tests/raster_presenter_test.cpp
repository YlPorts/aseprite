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
  // The viewport used by input mapping must exactly match presentation.
  const auto centered=rasterViewport(2,2,6,4,2);
  assert(centered.x==1 && centered.y==0);
  assert(centered.width==4 && centered.height==4);
  const auto narrow=rasterViewport(2,2,4,8,2);
  assert(narrow.x==0 && narrow.y==2);
  const auto nativeSize=rasterViewport(320,180,640,360,2);
  assert(nativeSize.x==0 && nativeSize.y==0);
  assert(rasterViewport(0,2,6,4,2).width==0);
  // On ultrawide windows we must leave neutral bars, not smear edge pixels.
  std::vector<uint8_t> wider(8*4*4,0xcd);
  WritePixels wide{wider.data(),6,4,32};
  assert(blitNearest(a,wide,2,{0,0,6,4}));
  auto solidBlack=[&](int x,int y){
    int i=y*32+x*4;
    assert(wider[i]==0 && wider[i+1]==0 && wider[i+2]==0 && wider[i+3]==255);
  };
  solidBlack(0,0);
  solidBlack(5,3);
  assert(wider[4]==1 && wider[5]==2 && wider[6]==3);
  assert(wider[4*4]==4 && wider[4*4+1]==5 && wider[4*4+2]==6);
  assert(wider[6*4]==0xcd);
  uint8_t rgba[]={20,30,40,255};
  uint8_t out[4]={};
  assert(blitNearest({rgba,1,1,4,PixelOrder::RGBA},
                     {out,1,1,4},1,{0,0,1,1}));
  assert(out[0]==20 && out[1]==30 && out[2]==40 && out[3]==255);
  // Phone UI scales: odd safe-area dimensions leave only neutral remainder
  // pixels. Each original pixel still becomes one uniform integer block.
  for(int scale:{4,5}) {
    const int width=2*scale+3,height=2*scale+5,stride=(width+2)*4;
    std::vector<uint8_t> phone(stride*height,0xcd);
    assert(blitNearest(a,{phone.data(),width,height,stride},scale,{0,0,width,height}));
    const auto viewport=rasterViewport(2,2,width,height,scale);
    for(int sy=0;sy<2;++sy) for(int sx=0;sx<2;++sx)
      for(int by=0;by<scale;++by) for(int bx=0;bx<scale;++bx) {
        const int offset=(viewport.y+sy*scale+by)*stride+(viewport.x+sx*scale+bx)*4;
        const int expected=sy*12+sx*4;
        assert(phone[offset]==src[expected+2] && phone[offset+1]==src[expected+1] &&
               phone[offset+2]==src[expected] && phone[offset+3]==255);
      }
    for(int y=0;y<height;++y) assert(phone[y*stride+width*4]==0xcd);
  }
  std::cout << "PASS: nearest neighbor, letterbox, BGRA/RGBA, stride, clipping\n";
}

#pragma once
#include <algorithm>
#include <vector>

namespace aseandroid {
struct FlowSize { int width,height; };
struct FlowCell { int x,y,width,height; };
struct FlowRows { std::vector<FlowCell> cells; int height=0; };

// Pack the ORIGINAL context-bar widgets into rows; retain their order and
// handlers. Zero width requests the natural single-row size hint.
inline FlowRows flowRows(const std::vector<FlowSize>& sizes,int width,int gap) {
  FlowRows result;
  int x=0,y=0,rowHeight=0;
  for(const auto& size:sizes) {
    const int w=width>0?std::min(size.width,width):size.width;
    if(width>0 && x>0 && x+w>width) {
      x=0;
      y+=rowHeight+gap;
      rowHeight=0;
    }
    result.cells.push_back({x,y,w,size.height});
    x+=w+gap;
    rowHeight=std::max(rowHeight,size.height);
  }
  result.height=y+rowHeight;
  return result;
}
}

#include "mobile_layout.h"
#include <cassert>
#include <iostream>
using namespace aseandroid;
int main() {
  const std::vector<FlowSize> tools{{16,18},{30,20},{16,18},{60,20},{30,18}};
  for(int width:{96,144,216,320,640}) {
    const auto rows=flowRows(tools,width,2);
    assert(rows.cells.size()==tools.size());
    for(size_t i=0;i<rows.cells.size();++i) {
      const auto& cell=rows.cells[i];
      assert(cell.x>=0 && cell.x+cell.width<=width);
      assert(cell.y>=0 && cell.y+cell.height<=rows.height);
      assert(cell.width==tools[i].width && cell.height==tools[i].height);
      for(size_t j=0;j<i;++j) {
        const auto& prior=rows.cells[j];
        assert(cell.x>=prior.x+prior.width || prior.x>=cell.x+cell.width ||
               cell.y>=prior.y+prior.height || prior.y>=cell.y+cell.height);
      }
    }
  }
  assert(flowRows(tools,0,2).height==20);
  assert(flowRows(tools,144,2).height==40);
  assert(flowRows({},216,2).height==0);
  const auto longField=flowRows({{320,20},{16,18}},216,2);
  assert(longField.cells[0].width==216 && longField.cells[1].y==22);
  std::cout<<"PASS: original tool options remain ordered, in bounds and nonoverlapping in portrait\n";
}

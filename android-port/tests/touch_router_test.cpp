#include "touch_router.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace aseandroid;
static Contact contact(int id, float x, float y, float p=1, Tool tool=Tool::Finger) {
  return {id,x,y,p,tool};
}
static Frame frame(Action a, int id, std::initializer_list<Contact> cs) {
  return {a,id,cs};
}
int main() {
  TouchRouter router;
  auto output = router.process(frame(Action::Down,0,{contact(0,10,11,.7f)}));
  assert(output.size()==1 && output[0].type==GestureType::Down);
  assert(std::fabs(output[0].pressure-.7f)<.001f);
  output=router.process(frame(Action::Move,0,{contact(0,11,12,2.f)}));
  assert(output.size()==1 && output[0].type==GestureType::Move);
  assert(output[0].pressure==1);
  output=router.process(frame(Action::PointerDown,1,{contact(0,11,12),contact(1,31,12)}));
  assert(output.size()==2 && output[0].type==GestureType::Up);
  assert(output[1].type==GestureType::PanStart);
  output=router.process(frame(Action::Move,1,{contact(0,11,12),contact(1,51,12)}));
  assert(output.size()==2 && output[0].type==GestureType::PanMove);
  assert(output[1].type==GestureType::Magnify);
  assert(std::fabs(output[1].magnification-1.f)<.001f);
  output=router.process(frame(Action::PointerUp,1,{contact(0,11,12),contact(1,51,12)}));
  assert(output.size()==1 && output[0].type==GestureType::PanEnd);
  assert(router.process(frame(Action::Move,0,{contact(0,14,14)})).empty());
  assert(router.process(frame(Action::Up,0,{contact(0,14,14)})).empty());
  output=router.process(frame(Action::Down,3,{contact(3,5,6,.4f,Tool::Pen)}));
  assert(output.size()==1 && output[0].tool==Tool::Pen);
  output=router.process(frame(Action::Cancel,3,{contact(3,5,6)}));
  assert(output.size()==1 && output[0].type==GestureType::Up);
  output=router.process(frame(Action::Down,4,{contact(4,0,0)}));
  assert(output.size()==1 && output[0].type==GestureType::Down);
  output=router.process(frame(Action::Up,4,{contact(4,2,3)}));
  assert(output.size()==1 && output[0].type==GestureType::Up && output[0].x==2);
  std::cout << "PASS: drawing, pressure, pinch, pan, stylus, cancel, release\n";
}

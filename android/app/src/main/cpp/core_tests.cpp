#include "editor.h"
#include <stdexcept>
#include <iostream>
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{
  mobile::Editor e(16,16);require(e.render()[0]==0,"transparent canvas");
  e.beginStroke();e.line(0,0,15,15,0xffff0033,1,false);require(e.render()[0]==0xffff0033,"ARGB conversion");
  auto file=e.save();mobile::Editor other(1,1);other.open(file);require(other.info()[0]==16&&other.render()==e.render(),"Aseprite format roundtrip");
  require(e.command(0,0)&&e.render()[0]==0,"undo");require(e.command(1,0)&&e.render()[0]==0xffff0033,"redo");
  e.command(2,0);require(e.info()[2]==2&&e.render()[0]==0xffff0033,"duplicate frame");e.beginStroke();e.line(0,0,0,0,0xff00ff00,1,false);e.command(4,0);require(e.render()[0]==0xffff0033,"frame independence");
  e.command(3,0);require(e.info()[4]==2,"add layer");e.beginStroke();e.line(0,0,0,0,0xff112233,1,false);require(e.render()[0]==0xff112233,"layer composite");e.command(6,0);require(e.render()[0]==0xffff0033,"hide layer");
  auto saved=e.save();other.open(saved);require(other.info()[2]==2&&other.info()[4]==2,"layer and frame roundtrip");
  bool rejected=false;try{other.open({1,2,3});}catch(...){rejected=true;}require(rejected&&other.render()==e.render(),"invalid input leaves document intact");
  std::cout<<"PASS: native Aseprite core; RGBA; drawing; format; undo/redo; layers; frames; rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

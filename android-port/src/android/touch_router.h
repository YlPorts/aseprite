#pragma once
#include <vector>

namespace aseandroid {
enum class Tool { Finger, Pen, Eraser, Mouse };
enum class Action { Down, PointerDown, Move, PointerUp, Up, Cancel };
enum class GestureType { Down, Move, Up, PanStart, PanMove, PanEnd, Magnify };
struct Contact { int id; float x, y, pressure; Tool tool; };
struct Frame {
  Action action;
  int actionId;
  // Contains all pointers including the one being lifted.
  std::vector<Contact> contacts;
};
struct Gesture {
  GestureType type;
  float x = 0, y = 0, pressure = 0;
  Tool tool = Tool::Finger;
  float magnification = 0;
};
class TouchRouter {
public:
  std::vector<Gesture> process(const Frame& frame);
  void reset();
private:
  enum class Mode { Idle, Drawing, Gesture };
  Mode mode_ = Mode::Idle;
  int primaryId_ = -1;
  Contact last_ = {-1, 0, 0, 0, Tool::Finger};
  bool panning_ = false;
  float cx_ = 0, cy_ = 0, distance_ = 0;
};
} // namespace aseandroid

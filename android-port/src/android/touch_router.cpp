#include "touch_router.h"
#include <algorithm>
#include <cmath>

namespace aseandroid {
namespace {
float clampPressure(float p) { return std::max(0.0f, std::min(1.0f, p)); }
Gesture point(GestureType type, const Contact& contact) {
  Gesture result;
  result.type = type;
  result.x = contact.x;
  result.y = contact.y;
  result.pressure = clampPressure(contact.pressure);
  result.tool = contact.tool;
  return result;
}
const Contact* find(const std::vector<Contact>& contacts, int id) {
  for (const auto& c : contacts) if (c.id == id) return &c;
  return nullptr;
}
} // namespace
void TouchRouter::reset() {
  mode_ = Mode::Idle;
  primaryId_ = -1;
  panning_ = false;
  cx_ = cy_ = distance_ = 0;
}
std::vector<Gesture> TouchRouter::process(const Frame& frame) {
  std::vector<Gesture> output;
  if (frame.action == Action::Cancel) {
    if (mode_ == Mode::Drawing) output.push_back(point(GestureType::Up, last_));
    if (panning_) output.push_back({GestureType::PanEnd, cx_, cy_});
    reset();
    return output;
  }
  std::vector<Contact> active;
  for (const Contact& c : frame.contacts) {
    if ((frame.action == Action::Up || frame.action == Action::PointerUp) &&
        c.id == frame.actionId) continue;
    active.push_back(c);
  }
  if (mode_ == Mode::Drawing) {
    if (active.size() >= 2) {
      output.push_back(point(GestureType::Up, last_));
      primaryId_ = -1;
      mode_ = Mode::Gesture;
    }
    else if (const Contact* c = find(active, primaryId_)) {
      last_ = *c;
      if (frame.action == Action::Move)
        output.push_back(point(GestureType::Move, *c));
      return output;
    }
    else {
      const Contact* c = find(frame.contacts, primaryId_);
      output.push_back(point(GestureType::Up, c ? *c : last_));
      reset();
      return output;
    }
  }
  if (mode_ == Mode::Idle && active.size() == 1 &&
      frame.action == Action::Down) {
    primaryId_ = active.front().id;
    last_ = active.front();
    mode_ = Mode::Drawing;
    output.push_back(point(GestureType::Down, last_));
    return output;
  }
  if (mode_ == Mode::Idle && active.size() >= 2)
    mode_ = Mode::Gesture;
  if (mode_ == Mode::Gesture) {
    if (active.size() >= 2) {
      const auto& a = active[0];
      const auto& b = active[1];
      const float x = (a.x + b.x) * 0.5f;
      const float y = (a.y + b.y) * 0.5f;
      const float d = std::hypot(a.x - b.x, a.y - b.y);
      if (!panning_) {
        output.push_back({GestureType::PanStart, x, y});
        panning_ = true;
      }
      else if (frame.action == Action::Move) {
        if (x != cx_ || y != cy_)
          output.push_back({GestureType::PanMove, x, y});
        if (distance_ > 1 && d > 1) {
          const float delta = d / distance_ - 1;
          if (std::fabs(delta) > 0.00001f) {
            Gesture zoom{GestureType::Magnify, x, y};
            zoom.magnification = delta;
            output.push_back(zoom);
          }
        }
      }
      cx_ = x;
      cy_ = y;
      distance_ = d;
    }
    else {
      if (panning_) output.push_back({GestureType::PanEnd, cx_, cy_});
      panning_ = false;
      distance_ = 0;
      // Suppress ghost strokes until all fingers are lifted.
      if (active.empty()) reset();
    }
  }
  return output;
}
} // namespace aseandroid

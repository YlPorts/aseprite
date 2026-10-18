#include "laf_event_bridge.h"
#include "os/event.h"
#include "os/surface.h"
#include "os/keys.h"
#include "os/android/key_state.h"
#include "gfx/point.h"
#include "raster_presenter.h"
#include <algorithm>
#include <cmath>
namespace aseandroid {
namespace {
os::PointerType pointer(Tool tool) {
  switch (tool) {
    case Tool::Pen: return os::PointerType::Pen;
    case Tool::Eraser: return os::PointerType::Eraser;
    case Tool::Mouse: return os::PointerType::Mouse;
    default: return os::PointerType::Touch;
  }
}
void space(os::Event::Type type, const os::WindowRef& window) {
  os::Event event;
  event.setType(type);
  event.setWindow(window);
  event.setScancode(os::kKeySpace);
  os::androidSetKeyPressed(os::kKeySpace, type==os::Event::KeyDown);
  os::queue_event(event);
}
gfx::Point logicalPosition(const Gesture& g,const os::WindowRef& window) {
  // Original LAF x11/win backends divide physical touch/mouse coordinates
  // by window UI scale before queuing Mouse/TouchMagnify events.
  const int scale=std::max(1,window->scale());
  const int dstW=window->width(),dstH=window->height();
  // Use the actual original Skia window surface, not an inferred size,
  // because SurfaceView resizing can precede Aseprite's paint resize.
  auto* surface=window->surface();
  const int srcW=surface?surface->width():std::max(1,dstW/scale);
  const int srcH=surface?surface->height():std::max(1,dstH/scale);
  const auto viewport=rasterViewport(srcW,srcH,dstW,dstH,scale);
  // Negative coordinates in the black letterbox must stay negative.
  const auto floorDivide=[](int value,int divisor)->int {
    return value>=0 ? value/divisor : -((-(int64_t(value))+divisor-1)/divisor);
  };
  return gfx::Point(floorDivide(static_cast<int>(std::lround(g.x))-viewport.x,scale),
                    floorDivide(static_cast<int>(std::lround(g.y))-viewport.y,scale));
}
void mouse(os::Event::Type type, const Gesture& g, const os::WindowRef& window, bool pan) {
  os::Event event;
  event.setType(type);
  event.setWindow(window);
  event.setPosition(logicalPosition(g,window));
  event.setPointerType(pointer(g.tool));
  event.setPressure(g.pressure);
  event.setButton(os::Event::LeftButton);
  if (pan) event.setModifiers(os::kKeySpaceModifier);
  os::queue_event(event);
}
} // namespace
void postToLaf(const std::vector<Gesture>& gestures, const os::WindowRef& window) {
  if (!window) return;
  for (const auto& g : gestures) {
    switch (g.type) {
      case GestureType::Down: mouse(os::Event::MouseDown,g,window,false); break;
      case GestureType::Move: mouse(os::Event::MouseMove,g,window,false); break;
      case GestureType::Up: mouse(os::Event::MouseUp,g,window,false); break;
      case GestureType::PanStart:
        space(os::Event::KeyDown,window);
        mouse(os::Event::MouseDown,g,window,true);
        break;
      case GestureType::PanMove: mouse(os::Event::MouseMove,g,window,true); break;
      case GestureType::PanEnd:
        mouse(os::Event::MouseUp,g,window,true);
        space(os::Event::KeyUp,window);
        break;
      case GestureType::Magnify: {
        os::Event event;
        event.setType(os::Event::TouchMagnify);
        event.setWindow(window);
        event.setPosition(gfx::Point(std::lround(g.x),std::lround(g.y)));
        event.setPointerType(os::PointerType::Touch);
        event.setMagnification(g.magnification);
        os::queue_event(event);
        break;
      }
    }
  }
}
} // namespace aseandroid

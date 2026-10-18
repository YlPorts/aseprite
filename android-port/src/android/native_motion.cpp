#include "native_motion.h"
#ifdef __ANDROID__
namespace aseandroid {
namespace {
Tool tool(int id) {
  switch (id) {
    case AMOTION_EVENT_TOOL_TYPE_STYLUS: return Tool::Pen;
    case AMOTION_EVENT_TOOL_TYPE_ERASER: return Tool::Eraser;
    case AMOTION_EVENT_TOOL_TYPE_MOUSE: return Tool::Mouse;
    default: return Tool::Finger;
  }
}
} // namespace
std::vector<Gesture> decodeMotionEvent(const AInputEvent* event, TouchRouter& router) {
  if (!event || AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
    return {};
  const int packedAction=AMotionEvent_getAction(event);
  const int kind=packedAction & AMOTION_EVENT_ACTION_MASK;
  const size_t index=(packedAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
  Frame frame;
  switch (kind) {
    case AMOTION_EVENT_ACTION_DOWN: frame.action=Action::Down; break;
    case AMOTION_EVENT_ACTION_POINTER_DOWN: frame.action=Action::PointerDown; break;
    case AMOTION_EVENT_ACTION_MOVE: frame.action=Action::Move; break;
    case AMOTION_EVENT_ACTION_POINTER_UP: frame.action=Action::PointerUp; break;
    case AMOTION_EVENT_ACTION_UP: frame.action=Action::Up; break;
    case AMOTION_EVENT_ACTION_CANCEL: frame.action=Action::Cancel; break;
    default: return {};
  }
  const size_t count=AMotionEvent_getPointerCount(event);
  if (count==0 || index>=count) {
    if (kind==AMOTION_EVENT_ACTION_CANCEL) return router.process(frame);
    return {};
  }
  frame.actionId=AMotionEvent_getPointerId(event,index);
  for (size_t i=0; i<count; ++i) {
    frame.contacts.push_back({
      AMotionEvent_getPointerId(event,i),
      AMotionEvent_getX(event,i),
      AMotionEvent_getY(event,i),
      AMotionEvent_getPressure(event,i),
      tool(AMotionEvent_getToolType(event,i))
    });
  }
  return router.process(frame);
}
} // namespace aseandroid
#endif

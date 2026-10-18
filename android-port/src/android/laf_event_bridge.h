#pragma once
#include "touch_router.h"
#include "os/window.h"
#include "os/event_queue.h"
namespace aseandroid {
// Requires original LAF headers and a real WindowAndroid to be linked.
void postToLaf(const std::vector<Gesture>& gestures, const os::WindowRef& window);
}

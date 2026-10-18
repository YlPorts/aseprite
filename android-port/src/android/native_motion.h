#pragma once
#ifdef __ANDROID__
#include <android/input.h>
#include "touch_router.h"
namespace aseandroid {
// Used by the future native Android window's input callback.
std::vector<Gesture> decodeMotionEvent(const AInputEvent* event, TouchRouter& router);
}
#endif

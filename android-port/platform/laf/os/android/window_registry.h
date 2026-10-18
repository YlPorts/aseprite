#pragma once
#include "os/window.h"
namespace os {
// JNI -> genuine original LAF window with a guarded owning reference.
void androidRegisterWindow(const WindowRef& window);
void androidUnregisterWindow(Window* window);
WindowRef androidAcquireWindow();
}

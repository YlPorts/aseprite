#include "os/android/window_registry.h"
#include <mutex>
namespace os {
namespace {
std::mutex& registryMutex() { static std::mutex mu; return mu; }
WindowRef& registryWindow() { static WindowRef current; return current; }
}
void androidRegisterWindow(const WindowRef& window) {
  std::lock_guard<std::mutex> lock(registryMutex());
  if (!registryWindow()) registryWindow()=window;
}
void androidUnregisterWindow(Window* window) {
  std::lock_guard<std::mutex> lock(registryMutex());
  if (window==nullptr || registryWindow().get()==window) registryWindow()=nullptr;
}
WindowRef androidAcquireWindow() {
  std::lock_guard<std::mutex> lock(registryMutex());
  return registryWindow();
}
}

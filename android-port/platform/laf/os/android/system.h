#pragma once
#include "os/common/system.h"
#include "os/android/screen.h"
#include "os/android/key_state.h"
namespace os {
// Only advertises platform capabilities that have been implemented.
class SystemAndroid : public CommonSystem {
public:
  bool isKeyPressed(KeyScancode code) override { return androidIsKeyPressed(code); }
  void resetKeyPressed() override { androidClearPressedKeys(); }
  gfx::Point mousePosition() const override { return androidPointerPosition(); }
  void setMousePosition(const gfx::Point& point) override {
    androidSetPointerPosition(point.x,point.y);
  }
  ScreenRef primaryScreen() override { return androidPrimaryScreen(); }
  void listScreens(ScreenList& screens) override { screens.push_back(primaryScreen()); }
};
} // namespace os

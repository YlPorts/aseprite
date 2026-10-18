#pragma once
#include "os/screen.h"
namespace os {
class ScreenAndroid final : public Screen {
public:
  bool isPrimary() const override { return true; }
  gfx::Rect bounds() const override;
  gfx::Rect workarea() const override { return bounds(); }
  ColorSpaceRef colorSpace() const override { return nullptr; }
  void* nativeHandle() const override;
};
// Share one stable screen identity across System and Window, like desktop LAF.
ScreenRef androidPrimaryScreen();
} // namespace os

#pragma once
#include "os/common/system.h"
#include "os/android/screen.h"
namespace os {
// Only advertises platform capabilities that have been implemented.
class SystemAndroid : public CommonSystem {
public:
  ScreenRef primaryScreen() override { return make_ref<ScreenAndroid>(); }
  void listScreens(ScreenList& screens) override { screens.push_back(primaryScreen()); }
};
} // namespace os

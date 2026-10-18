#pragma once
#ifdef __ANDROID__
#include "raster_presenter.h"
#include <android/native_window.h>
#include <mutex>
namespace aseandroid {
class AndroidRasterWindow {
public:
  ~AndroidRasterWindow();
  void setSurface(ANativeWindow* surface);
  bool present(const ReadPixels& pixels,int uiScale);
  bool hasSurface() const;
  void dimensions(int& width,int& height) const;
  ANativeWindow* nativeHandle() const; // observation only
private:
  mutable std::mutex mutex_;
  ANativeWindow* surface_=nullptr; // acquired reference
};
AndroidRasterWindow& globalRasterWindow();
}
#endif

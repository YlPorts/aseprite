#include "android_raster_window.h"
#ifdef __ANDROID__
#include <android/native_window.h>
#include <limits>
namespace aseandroid {
AndroidRasterWindow::~AndroidRasterWindow(){setSurface(nullptr);}
AndroidRasterWindow& globalRasterWindow(){static AndroidRasterWindow surface;return surface;}
void AndroidRasterWindow::dimensions(int& width,int& height) const {
  std::lock_guard<std::mutex> lock(mutex_);
  width=surface_?ANativeWindow_getWidth(surface_):0;
  height=surface_?ANativeWindow_getHeight(surface_):0;
}
ANativeWindow* AndroidRasterWindow::nativeHandle() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return surface_;
}
void AndroidRasterWindow::setSurface(ANativeWindow* surface) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (surface==surface_) return;
  if (surface) {
    ANativeWindow_acquire(surface);
    ANativeWindow_setBuffersGeometry(surface,0,0,WINDOW_FORMAT_RGBA_8888);
  }
  ANativeWindow* old=surface_;
  surface_=surface;
  if(old) ANativeWindow_release(old);
}
bool AndroidRasterWindow::hasSurface() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return surface_!=nullptr;
}
bool AndroidRasterWindow::present(const ReadPixels& pixels,int uiScale) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!surface_ || uiScale<1 || uiScale>16) return false;
  ANativeWindow_Buffer buffer{};
  if (ANativeWindow_lock(surface_,&buffer,nullptr)<0) return false;
  // The Android buffer stride is in pixels, not bytes. Validate it
  // before multiplying to prevent overflow or writing past a row.
  const bool layoutOK=buffer.bits && buffer.width>0 && buffer.height>0 &&
    buffer.stride>=buffer.width &&
    buffer.stride<=std::numeric_limits<int>::max()/4;
  const bool ok=layoutOK && (buffer.format==WINDOW_FORMAT_RGBA_8888) &&
    blitNearest(pixels,{static_cast<uint8_t*>(buffer.bits),buffer.width,
                          buffer.height,buffer.stride*4},uiScale,
                 {0,0,buffer.width,buffer.height});
  const int status=ANativeWindow_unlockAndPost(surface_);
  return ok && status>=0;
}
}
#endif

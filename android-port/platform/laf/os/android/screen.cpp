#include "os/android/screen.h"
#include "android_raster_window.h"
namespace os {
gfx::Rect ScreenAndroid::bounds() const {
  int width=0,height=0;
  aseandroid::globalRasterWindow().dimensions(width,height);
  return gfx::Rect(0,0,width,height);
}
void* ScreenAndroid::nativeHandle() const {
  return aseandroid::globalRasterWindow().nativeHandle();
}
}

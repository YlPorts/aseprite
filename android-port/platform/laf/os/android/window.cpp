#include "os/android/window.h"
#include "os/android/screen.h"
#include "os/android/key_state.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "android_raster_window.h"
#include "android_ime_bridge.h"
#include <algorithm>
namespace os {
WindowAndroid::WindowAndroid(const WindowSpec& spec)
  : m_scale(std::max(1,std::min(16,spec.scale()))) { }
gfx::Rect WindowAndroid::contentRect() const {
  return gfx::Rect(0,0,width(),height());
}
void WindowAndroid::setFrame(const gfx::Rect& bounds) {
  (void)bounds; // Fullscreen native surface is controlled by Android.
}
int WindowAndroid::width() const {
  int w=0,h=0;
  aseandroid::globalRasterWindow().dimensions(w,h);
  return w;
}
int WindowAndroid::height() const {
  int w=0,h=0;
  aseandroid::globalRasterWindow().dimensions(w,h);
  return h;
}
gfx::Size WindowAndroid::clientSize() const { return gfx::Size(width(),height()); }
void WindowAndroid::setScale(int scale) {
  scale=std::max(1,std::min(16,scale));
  if(m_scale==scale) return;
  m_scale=scale;
  if(width()>0 && height()>0) onResize(clientSize());
}
void WindowAndroid::setTextInput(bool enabled, const gfx::Point& caretPos) {
  (void)caretPos; // The platform IME is anchored to the focused SurfaceView.
  aseandroid::requestOriginalEditorIme(enabled);
}
void WindowAndroid::invalidateRegion(const gfx::Region& rgn) {
  (void)rgn; // Present the full Skia bitmap; dirty rect optimization follows.
  if (!isVisible() || width()<=0 || height()<=0) return;
  if (m_paintQueued.exchange(true,std::memory_order_acq_rel)) return;
  // A callback executes via the ORIGINAL Aseprite UI event queue, never
  // synchronously inside a drawing operation or on the Android Java thread.
  // Keep the real Window object alive until this queued paint completes.
  auto self=AddRef(this);
  os::Event redraw;
  redraw.setType(os::Event::Callback);
  redraw.setWindow(self);
  redraw.setCallback([self] {
    auto* window=static_cast<WindowAndroid*>(self.get());
    window->m_paintQueued.store(false,std::memory_order_release);
    if(window->isVisible() && window->width()>0 && window->height()>0)
      window->onPaint(window->bounds());
  });
  os::queue_event(redraw);
}
ScreenRef WindowAndroid::screen() const { return androidPrimaryScreen(); }
void WindowAndroid::setMousePosition(const gfx::Point& position) {
  // The caller supplies logical coordinates; System stores physical pixels.
  androidSetPointerPosition(position.x*scale(), position.y*scale());
}
Window::NativeHandle WindowAndroid::nativeHandle() const {
  return aseandroid::globalRasterWindow().nativeHandle();
}
void WindowAndroid::notifySurfaceChanged() {
  if(width()>0 && height()>0) {
    onResize(clientSize());
    // Repaint an existing editor after Android recreates its SurfaceView,
    // even if no sprite pixels changed while the screen was detached.
    invalidate();
  }
}
} // namespace os

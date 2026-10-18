#pragma once
#include "gfx/rect.h"
#include "gfx/size.h"
#include "os/native_cursor.h"
#include "os/window.h"
#include "os/window_spec.h"
#include <string>
namespace os {
// Main Android surface facade conforming to original LAF Window interface.
// Android itself resizes/owns the fullscreen window.
class WindowAndroid : public Window {
public:
  explicit WindowAndroid(const WindowSpec& spec);
  ~WindowAndroid() override = default;
  gfx::Rect frame() const override { return contentRect(); }
  void setFrame(const gfx::Rect& bounds) override;
  gfx::Rect contentRect() const override;
  gfx::Rect restoredFrame() const override { return contentRect(); }
  int width() const override;
  int height() const override;
  gfx::Size clientSize() const;
  int scale() const override { return m_scale; }
  void setScale(int scale) override;
  bool isVisible() const override { return m_visible; }
  void setVisible(bool visible) override { m_visible=visible; }
  void invalidateRegion(const gfx::Region& rgn) override;
  // Required by the genuine SkiaSystem::setTextInput() call path.
  // The future IME adapter must forward this state to Android's input method.
  void setTextInput(bool enabled,const gfx::Point& caretPos = {}) {
    (void)enabled; (void)caretPos;
  }
  void activate() override { }
  void maximize() override { }
  void minimize() override { }
  bool isMaximized() const override { return true; }
  bool isMinimized() const override { return !m_visible; }
  bool isTransparent() const override { return false; }
  bool isFullscreen() const override { return true; }
  void setFullscreen(bool state) override { (void)state; }
  std::string title() const override { return m_title; }
  void setTitle(const std::string& title) override { m_title=title; }
  NativeCursor nativeCursor() override { return m_cursor; }
  bool setCursor(NativeCursor cursor) override { m_cursor=cursor; return true; }
  bool setCursor(const CursorRef& cursor) override { (void)cursor; return false; }
  void setMousePosition(const gfx::Point& position) override;
  void captureMouse() override { }
  void releaseMouse() override { }
  void performWindowAction(WindowAction action, const Event* event) override {
    (void)action; (void)event;
  }
  std::string getLayout() override { return std::string(); }
  void setLayout(const std::string&) override { }
  ScreenRef screen() const override;
  void setColorSpace(const ColorSpaceRef& space) override { (void)space; }
  NativeHandle nativeHandle() const override;
  // Called on the actual UI thread when the surface is attached/resized.
  void notifySurfaceChanged();
protected:
  virtual void onPaint(const gfx::Rect& rect)=0;
  virtual void onResize(const gfx::Size& size)=0;
private:
  std::string m_title="Aseprite";
  int m_scale=1;
  bool m_visible=true;
  NativeCursor m_cursor=NativeCursor::Arrow;
};
} // namespace os

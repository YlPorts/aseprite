// Android JNI OS host: calls ORIGINAL Aseprite app_main(), not another editor.
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "android_raster_window.h"
#include "touch_router.h"
#include "laf_event_bridge.h"
#include "os/android/window_registry.h"
#include "os/android/key_state.h"
#include "os/android/window.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/keys.h"

extern int app_main(int argc,char* argv[]); // Original src/main/main.cpp
namespace {
std::once_flag startOnce;
std::mutex touchMutex;
aseandroid::TouchRouter touchRouter;
std::atomic_bool originalEditorStarted{false};

void startOriginalAseprite(std::string filesRoot) {
  const std::string data=filesRoot+"/data";
  const std::string user=filesRoot+"/user";
  setenv("HOME",filesRoot.c_str(),1);
  setenv("ASEPRITE_ANDROID_DATA_DIR",data.c_str(),1);
  setenv("ASEPRITE_USER_FOLDER",user.c_str(),1);
  char name[]="aseprite";
  char* argv[]={name,nullptr};
  originalEditorStarted.store(true,std::memory_order_release);
  try {
    (void)app_main(1,argv);
  } catch (...) {
    __android_log_print(ANDROID_LOG_ERROR,"AsepriteAndroid",
                        "Original Aseprite app_main terminated with exception");
  }
  originalEditorStarted.store(false,std::memory_order_release);
  os::androidClearPressedKeys();
  os::androidUnregisterWindow(nullptr);
}
}

extern "C" JNIEXPORT void JNICALL
Java_com_ylports_aseprite_AsepriteActivity_nativeAttachSurface(
    JNIEnv* env,jclass,jobject surface) {
  ANativeWindow* native=surface?ANativeWindow_fromSurface(env,surface):nullptr;
  aseandroid::globalRasterWindow().setSurface(native);
  if(native) ANativeWindow_release(native);
  // Send resize through the genuine Aseprite/LAF UI thread.
  auto window=os::androidAcquireWindow();
  if(window && native) {
    os::Event event;
    event.setType(os::Event::Callback);
    event.setCallback([window] {
      auto* androidWindow=dynamic_cast<os::WindowAndroid*>(window.get());
      if(androidWindow) androidWindow->notifySurfaceChanged();
    });
    os::queue_event(event);
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_ylports_aseprite_AsepriteActivity_nativeStartOriginalEditor(
    JNIEnv* env,jclass,jstring filesRoot) {
  if(!filesRoot || !aseandroid::globalRasterWindow().hasSurface()) return;
  const char* value=env->GetStringUTFChars(filesRoot,nullptr);
  if(!value) return;
  std::string root(value);
  env->ReleaseStringUTFChars(filesRoot,value);
  if(root.empty()) return;
  std::call_once(startOnce,[&] {std::thread(startOriginalAseprite,root).detach();});
}

extern "C" JNIEXPORT void JNICALL
Java_com_ylports_aseprite_AsepriteActivity_nativeTouch(
    JNIEnv* env,jclass,jint action,jint index,jintArray ids,
    jfloatArray xy,jfloatArray pressure,jintArray tools) {
  if(!originalEditorStarted.load(std::memory_order_acquire) ||
     !ids || !xy || !pressure || !tools) return;
  const int n=env->GetArrayLength(ids);
  if(n<1 || n>16 || index<0 || index>=n ||
     env->GetArrayLength(xy)!=n*2 ||
     env->GetArrayLength(pressure)!=n ||
     env->GetArrayLength(tools)!=n) return;
  aseandroid::Action kind;
  switch(action) {
    case 0: kind=aseandroid::Action::Down; break;
    case 5: kind=aseandroid::Action::PointerDown; break;
    case 2: kind=aseandroid::Action::Move; break;
    case 6: kind=aseandroid::Action::PointerUp; break;
    case 1: kind=aseandroid::Action::Up; break;
    case 3: kind=aseandroid::Action::Cancel; break;
    default: return;
  }
  std::vector<jint> id(n),type(n);
  std::vector<jfloat> point(n*2),press(n);
  env->GetIntArrayRegion(ids,0,n,id.data());
  env->GetIntArrayRegion(tools,0,n,type.data());
  env->GetFloatArrayRegion(xy,0,n*2,point.data());
  env->GetFloatArrayRegion(pressure,0,n,press.data());
  if(env->ExceptionCheck()) return;
  aseandroid::Frame frame;
  frame.action=kind;
  frame.actionId=id[index];
  frame.contacts.reserve(n);
  for(int i=0;i<n;++i) {
    if(!std::isfinite(point[i*2]) || !std::isfinite(point[i*2+1])) return;
    aseandroid::Tool tool=aseandroid::Tool::Finger;
    if(type[i]==2) tool=aseandroid::Tool::Pen;
    else if(type[i]==4) tool=aseandroid::Tool::Eraser;
    else if(type[i]==3) tool=aseandroid::Tool::Mouse;
    frame.contacts.push_back({id[i],point[i*2],point[i*2+1],press[i],tool});
  }
  std::lock_guard<std::mutex> lock(touchMutex);
  const auto window=os::androidAcquireWindow();
  if(!window) return;
  aseandroid::postToLaf(touchRouter.process(frame),window);
}

extern "C" JNIEXPORT void JNICALL
Java_com_ylports_aseprite_AsepriteActivity_nativeKey(
    JNIEnv*,jclass,jint action,jint androidCode,jint unicode,jint meta) {
  if(!originalEditorStarted.load(std::memory_order_acquire)) return;
  auto window=os::androidAcquireWindow();
  if(!window) return;
  auto key=os::androidMapKey(androidCode);
  if(key==os::kKeyNil) return;
  const bool down=(action==0);
  os::androidSetKeyPressed(key,down);
  os::Event event;
  event.setType(down?os::Event::KeyDown:os::Event::KeyUp);
  event.setWindow(window);
  event.setScancode(key);
  event.setUnicodeChar(static_cast<base::codepoint_t>(unicode));
  os::KeyModifiers mods=os::kKeyNoneModifier;
  if(meta&0x01) mods=static_cast<os::KeyModifiers>(mods|os::kKeyShiftModifier);
  if(meta&0x1000) mods=static_cast<os::KeyModifiers>(mods|os::kKeyCtrlModifier);
  if(meta&0x02) mods=static_cast<os::KeyModifiers>(mods|os::kKeyAltModifier);
  event.setModifiers(mods);
  os::queue_event(event);
}

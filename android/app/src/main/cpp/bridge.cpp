#include "editor.h"
#include <jni.h>
#include <exception>
#include <stdexcept>
#include <vector>
namespace {
mobile::Editor& editor(jlong p){if(!p)throw std::runtime_error("Editor cerrado");return *reinterpret_cast<mobile::Editor*>(p);}
template<class F> auto guarded(JNIEnv* env,F f)->decltype(f()) {
  try{return f();}catch(const std::exception& e){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),e.what());}catch(...){env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),"Error nativo desconocido");}return {};
}
}
extern "C" {
JNIEXPORT jlong JNICALL Java_org_ylports_aseprite_NativeCore_create(JNIEnv* e,jclass,jint w,jint h){return guarded(e,[&]()->jlong{return reinterpret_cast<jlong>(new mobile::Editor(w,h));});}
JNIEXPORT jint JNICALL Java_org_ylports_aseprite_NativeCore_destroy(JNIEnv* e,jclass,jlong p){return guarded(e,[&]()->jint{delete reinterpret_cast<mobile::Editor*>(p);return 0;});}
JNIEXPORT jintArray JNICALL Java_org_ylports_aseprite_NativeCore_info(JNIEnv* e,jclass,jlong p){return guarded(e,[&](){auto v=editor(p).info();auto a=e->NewIntArray(v.size());if(a)e->SetIntArrayRegion(a,0,v.size(),v.data());return a;});}
JNIEXPORT jintArray JNICALL Java_org_ylports_aseprite_NativeCore_render(JNIEnv* e,jclass,jlong p){return guarded(e,[&](){auto v=editor(p).render();auto a=e->NewIntArray(v.size());if(a)e->SetIntArrayRegion(a,0,v.size(),reinterpret_cast<jint*>(v.data()));return a;});}
JNIEXPORT jint JNICALL Java_org_ylports_aseprite_NativeCore_begin(JNIEnv* e,jclass,jlong p){return guarded(e,[&]()->jint{editor(p).beginStroke();return 0;});}
JNIEXPORT jint JNICALL Java_org_ylports_aseprite_NativeCore_line(JNIEnv* e,jclass,jlong p,jint a,jint b,jint c,jint d,jint color,jint size,jboolean erase){return guarded(e,[&]()->jint{editor(p).line(a,b,c,d,uint32_t(color),size,erase);return 0;});}
JNIEXPORT jboolean JNICALL Java_org_ylports_aseprite_NativeCore_command(JNIEnv* e,jclass,jlong p,jint op,jint value){return guarded(e,[&]()->jboolean{return editor(p).command(op,value);});}
JNIEXPORT jstring JNICALL Java_org_ylports_aseprite_NativeCore_layers(JNIEnv* e,jclass,jlong p){return guarded(e,[&](){return e->NewStringUTF(editor(p).layerNames().c_str());});}
JNIEXPORT jbyteArray JNICALL Java_org_ylports_aseprite_NativeCore_save(JNIEnv* e,jclass,jlong p){return guarded(e,[&](){auto v=editor(p).save();auto a=e->NewByteArray(v.size());if(a)e->SetByteArrayRegion(a,0,v.size(),reinterpret_cast<jbyte*>(v.data()));return a;});}
JNIEXPORT jint JNICALL Java_org_ylports_aseprite_NativeCore_open(JNIEnv* e,jclass,jlong p,jbyteArray b){return guarded(e,[&]()->jint{if(!b)throw std::runtime_error("Archivo vacio");jsize n=e->GetArrayLength(b);if(n>32*1024*1024)throw std::runtime_error("Archivo demasiado grande");std::vector<uint8_t> v(n);e->GetByteArrayRegion(b,0,n,reinterpret_cast<jbyte*>(v.data()));if(!e->ExceptionCheck())editor(p).open(v);return 0;});}
}

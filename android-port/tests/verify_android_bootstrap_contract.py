#!/usr/bin/env python3
"""Verify the source-level Java <-> JNI contract and first-window ownership.

This is a static build gate, not a substitute for a device startup test.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
java=(root/"app/src/main/java/com/ylports/aseprite/AsepriteActivity.java").read_text()
jni=(root/"src/android/jni_editor_entry.cpp").read_text()
screen=(root/"platform/laf/os/android/screen.cpp").read_text()
system=(root/"platform/laf/os/android/system.h").read_text()
window=(root/"platform/laf/os/android/window.cpp").read_text()
bridge=(root/"src/android/laf_event_bridge.cpp").read_text()
raster=(root/"src/android/raster_presenter.cpp").read_text()

signature=re.search(
    r"private\s+static\s+native\s+boolean\s+nativeStartOriginalEditor\s*\(\s*String\s+filesRoot\s*\)",
    java)
entry=re.search(
    r'extern\s+"C"\s+JNIEXPORT\s+jboolean\s+JNICALL\s+'
    r'Java_com_ylports_aseprite_AsepriteActivity_nativeStartOriginalEditor\s*\(',
    jni)
assert signature and entry, "Java method and JNI native return type must agree"
assert java.count("nativeStartOriginalEditor(getFilesDir().getAbsolutePath())")==1
assert "if (nativeStartOriginalEditor(getFilesDir().getAbsolutePath()))" in java
assert "return dispatched ? JNI_TRUE : JNI_FALSE;" in jni
assert "app_main(1,argv)" in jni, "Must launch original Aseprite app_main"
assert "AsepriteAndroid" in jni and "Original editor app_main exited with code" in jni
assert "dynamic_cast<os::WindowAndroid*>" not in jni
assert "rasterViewport(" in bridge and "rasterViewport(" in raster
magnify=bridge.split('case GestureType::Magnify:',1)[-1]
assert "event.setPosition(logicalPosition(g,window));" in magnify, (
    "Pinch-zoom center must map to the original Skia logical viewport")
assert "surface->width()" in bridge and "surface->height()" in bridge
assert "static ScreenRef primary=make_ref<ScreenAndroid>();" in screen
assert "return androidPrimaryScreen();" in system
assert "return androidPrimaryScreen();" in window
assert "androidSetPointerPosition(position.x*scale(), position.y*scale())" in window
print("PASS: original editor JNI return contract, startup gating and screen identity")

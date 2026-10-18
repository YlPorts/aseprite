# Built from ORIGINAL Aseprite top-level CMake, after add_subdirectory(src).
if(NOT ANDROID)
  message(FATAL_ERROR "Original editor JNI target requires the Android NDK")
endif()
if(ENABLE_ASEPRITE_EXE)
  message(FATAL_ERROR "Disable desktop executable; Android uses shared original editor")
endif()
if(NOT TARGET app-lib OR NOT TARGET laf-os)
  message(FATAL_ERROR "Missing original Aseprite targets in recursive checkout")
endif()
if(NOT LAF_BACKEND STREQUAL "skia")
  message(FATAL_ERROR "Original editor requires its Skia renderer, not a fake UI")
endif()
# Build the touch module directly inside ORIGINAL Aseprite's CMake
# project. Re-entering android-port/CMakeLists.txt would run another project()
# and unexpectedly change the top-level CMake project context.
# Raster code is compiled ONCE inside laf-os by the guarded LAF overlay.
add_library(aseprite-android-touch STATIC
  ${CMAKE_SOURCE_DIR}/android-port/src/android/touch_router.cpp
  ${CMAKE_SOURCE_DIR}/android-port/src/android/native_motion.cpp)
target_include_directories(aseprite-android-touch PUBLIC
  ${CMAKE_SOURCE_DIR}/android-port/src/android)
target_link_libraries(aseprite-android-touch PUBLIC android)
add_library(aseprite SHARED
  ${CMAKE_SOURCE_DIR}/src/main/main.cpp
  ${CMAKE_SOURCE_DIR}/android-port/src/android/jni_editor_entry.cpp
  ${CMAKE_SOURCE_DIR}/android-port/src/android/laf_event_bridge.cpp)
# Android's dynamic loader requires the genuine app_main() and every
# referenced Aseprite/LAF/Skia symbol to resolve before the APK starts.
# Shared-library links otherwise allow undefined symbols and produce a
# seemingly valid .so that fails immediately in System.loadLibrary().
target_link_options(aseprite PRIVATE "-Wl,--no-undefined")
set_target_properties(aseprite PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_compile_definitions(aseprite PRIVATE HAVE_CONFIG_H)
target_include_directories(aseprite PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/laf
  ${CMAKE_SOURCE_DIR}/android-port/src/android)
target_link_libraries(aseprite PRIVATE app-lib
  aseprite-android-touch laf-os android log)
add_dependencies(aseprite copy_data)

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
# The raster presenter is already compiled into laf-os by the guarded
# platform overlay. Do not link its standalone archive again.
add_subdirectory(${CMAKE_SOURCE_DIR}/android-port ${CMAKE_BINARY_DIR}/android-port-native)
add_library(aseprite SHARED
  ${CMAKE_SOURCE_DIR}/src/main/main.cpp
  ${CMAKE_SOURCE_DIR}/android-port/src/android/jni_editor_entry.cpp
  ${CMAKE_SOURCE_DIR}/android-port/src/android/laf_event_bridge.cpp)
target_compile_definitions(aseprite PRIVATE HAVE_CONFIG_H)
target_include_directories(aseprite PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/laf
  ${CMAKE_SOURCE_DIR}/android-port/src/android)
target_link_libraries(aseprite PRIVATE app-lib
  aseprite-android-touch laf-os android log)
add_dependencies(aseprite copy_data)

# Android native port status — experimental (not yet an APK)

Original Aseprite source: `YlPorts/aseprite`, branch `android-native-port-wip`.

## Implemented in this branch

- Touch arbitration: single-finger paint, pressure, pen, eraser, dual-finger pan and pinch.
- Conversion to genuine `laf/os::Event` input types.
- Native NDK input decoder for `AInputEvent` (Activity hookup still missing).
- Thread-safe Android queue for the original LAF `EventQueue` API.
- **Raster Skia output**: 32-bit RGBA/BGRA pixels, neighbor-nearest integer scaling,
  row stride, clipped updates and native `ANativeWindow` lock/unlock/post.
- An experimental `ScreenAndroid`, `SystemAndroid`, `WindowAndroid` and
  `SkiaWindowAndroid` adapter as an overlay on the original LAF sources,
  without redrawing/reimplementing Aseprite's UI.
- Guarded overlay patch script `patch-laf-android-raster.py` to apply
  after `patch-laf-headless-android.py`.
- Ubuntu CTest and NDK `arm64-v8a` module checks, plus experimental CI
  for `doc-lib`, `render-lib`, and `dio-lib`.

## Actually verified

- In local Linux host build: **3/3 CTest** (touch, event queue, raster presentation).
- The raster backend, original LAF overlay, NDK ABI, and full editor remain
  **UNVERIFIED at runtime**. CI has been committed but no passing run has been
  confirmed through the available connector.
- No APK was compiled. No original Aseprite GUI function is claimed operational.

## New original-editor bootstrapping (source integrated, not verified)

- Android Java SurfaceView host in `android-port/app/` receives the real surface,
  pointer and keyboard events and extracts authentic `data/` resources.
- JNI entry in `src/android/jni_editor_entry.cpp` launches Aseprite's existing
  `app_main()` in its engine thread and delivers surface/resize/input events.
- An owning, synchronized original LAF window registry and Android keyboard
  state provide JNI-to-LAF reference and modifier handling.
- `android-port/native-app/CMakeLists.txt` defines a real shared `libaseprite.so`
  from the upstream `src/main/main.cpp` and `app-lib`.
- `patch-aseprite-android-engine.py` and `patch-laf-android-runtime.py` connect
  source overlays without modifying `main`.
- Gradle fails deliberately when `libaseprite.so` or original GUI assets
  are missing; this is not a runnable APK without the original engine.
- GitHub integration-preflight workflow checks patch compatibility only;
  not a completed original GUI build.

## Corrections and checks — current continuation

- Fix CMake dispatch selecting `skia_window_android.cpp` rather than X11 for Android.
- Make the headless/raster/runtime overlays re-applicable and add an
  isolated regression test against real source/submodule files:
  `tests/patch_laf_integration_test.py`.
- Release Android JNI window registry before `SkiaSystem` event teardown;
  do not advertise unsupported multiple native windows.
- Correct missing native `setTextInput()` interface and update the original
  `System::mousePosition()` pointer coordinate support.
- Avoid linking two copies of the standalone raster presenter.
- Add an experimental GitHub Actions job to build authentic Aseprite
  `aseprite-m124` Skia with Android NDK 27, ARM64 and CPU raster enabled.
- **Locally verified in this session:** the original stage-1 touch CTest
  passed (1/1) and the GitHub-sourced C++ raster test passed with
  AddressSanitizer + UndefinedBehaviorSanitizer.
- Checked 15 expected integration anchors in current official Aseprite/LAF
  sources via repository content inspection (not a compiled Android port).
- New GitHub Actions workflows and real editor JNI integration have no
  confirmed passing end-to-end run; do not claim an APK works.

## Known blockers

1. **Validate and fix** the new Activity/JNI `app_main()` integration in real NDK builds.
2. Compile the original `laf` Skia backend with Android-built Skia m124,
   including adjustment of renderer's native format/canvas sizing.
3. Validate event propagation; complete IME, mouse/stylus edge cases, dialogs and clipboard.
4. Compile/link original `app-lib`, generate resource headers with host `gen`.
5. Package authentic `data/` themes/fonts/icons/strings in APK assets.
6. SAF open/save, URI permission persistence, safe autosave/recovery.
7. Performance, window resizing/rotation, multiple panels, external inputs.
8. Verify with an ARM64 Android device and functionality checklist.

**Important limitations:** current window adapter supports the primary native
surface only. `invalidateRegion()` is deferred to the authentic
`ui::Display::flip()` → `swapBuffers()` path; finer dirty-region
scheduling, native dialogs, external windows, and EGL acceleration remain.
Android raster is CPU-based, not GPU-accelerated yet.

## License

Aseprite EULA restricts modifying and building source to personal use or
contributions, and prohibits distributing binaries. This fork is PUBLIC.
No compiled Aseprite binary should be published here. For private CI
artifacts, use a separate private repository (not a public GitHub fork).

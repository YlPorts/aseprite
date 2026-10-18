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

## Known blockers

1. Connect Android Activity/native input queue/lifecycle to `app_main()`.
2. Compile the original `laf` Skia backend with Android-built Skia m124,
   including adjustment of renderer's native format/canvas sizing.
3. Wire event propagation, complete keyboard and IME, native dialogs and clipboard.
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

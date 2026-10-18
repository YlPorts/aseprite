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

## Correcciones de integración verificadas en esta sesión

- **Parche raster**: `replace_once()` acepta ahora `accepted_later`, usado
  por la integración tras aplicar el backend de ventanas.
- **Parche `SkiaSystem`**: corregidas secuencias de salto de línea doblemente
  escapadas que producirían texto C++ inválido.
- **Skia GN**: establecido `target_os="android"` además de `target_cpu="arm64"`,
  y la CI comprueba explícitamente ambos valores antes de compilar.
- **Renderización ultrawide**: el reescalado entero usa márgenes negros
  cuando los tamaños físicos no coinciden, sin repetir píxeles de borde;
  pruebas nuevas para RGBA/BGRA y stride.
- **Teclado original**: añadido `android_key_state_test.cpp` y ejecución
  en el preflight de GitHub contra los tipos reales de `laf`.

**Comprobación ejecutada localmente:** compilación CMake de módulos aislados,
`ctest` 3/3 y prueba raster con AddressSanitizer/UBSan sin fallos detectados.
El editor original y el backend Android completo NO se han compilado ni
probado en el dispositivo. La CI no tiene resultados concluyentes confirmados.

## Nueva compilación integral (experimental)

El script `android-port/scripts/try-build-original-editor-arm64.sh`
intenta, tras compilar Skia m124 Android ARM64 en GitHub Actions:

- Ejecutar los cuatro parches sobre los fuentes originales.
- Construir `gen` en el host Linux (no en Android).
- Configurar CMake con NDK ARM64, Skia raster y PIC.
- Enlazar el `app_main()` auténtico, `app-lib` y `laf-os` para generar
  una biblioteca JNI genuina `libaseprite.so`.

Esto es un **intento reproducible**, NO un binario conseguido. La compilación
puede fallar mientras se corrigen dependencias/portabilidad y no tiene
resultados confirmados aquí. El workflow no crea/publica una APK.

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

## October 18 — original engine integration hardening

- Corrected Skia Android renderer: `SkiaSurface::bitmap()` is non-const in
  the pinned LAF interface; the renderer now calls it on a mutable surface
  and checks row-stride bounds.
- Converted physical Android motion positions to original **LAF logical
  coordinates** using `Window::scale()`, matching X11/Windows event behavior
  for non-1x editor UI scale.
- GitHub CI now attempts NDK ARM64 syntax compilation of JNI and **nine**
  Android/LAF adapter translation units before the full Skia linkage.
- Original-editor ARM64 build script builds host `gen` with clipboard optional
  and validates output ELF machine `AArch64` before claiming native success.
- Added `stage-authentic-android-assets.py` to prepare original `data/`
  assets only alongside a real AArch64 ELF library exposing the expected JNI
  boot entrypoint. It does **not** package or publish an APK.
- Added regression checks for malformed ELF / wrong CPU / absent JNI,
  wired into integration-preflight Actions. **New CI results not confirmed.**
- Existing standalone host unit suite still passes **3/3** locally.

This is real source work, not a fully linked or device-tested original
Aseprite editor. Full Skia ARM64/linker errors remain to resolve once a
CI run completes and its logs are inspected.


## Continuación: fallos concretos de compilación corregidos

- Se reemplazó el workflow Skia malformado (líneas duplicadas, `grep` roto)
  por una secuencia única con checkout recursivo del Aseprite original.
- La generación GN ahora utiliza el ejecutable `gn` de `depot_tools`.
- El software raster original `laf/os/skia/skia_surface.cpp` ya no intenta
  utilizar clases internas de GPU cuando `SK_SUPPORT_GPU=0`; la prueba de
  parches valida la condición sobre el código real del submódulo.
- Se excluyó la comprobación libc++ específica de Linux para el NDK Android.
- `laf/base/platform_unix.cpp` se protege con `LAF_LINUX` para no
  compilar utilidades exclusivas de distribuciones Linux en Android.
- Se eliminó una comprobación que exigía un inexistente
  `laf/clip/CMakeLists.txt` incluso con portapapeles deshabilitado.
- Se cancelan trazos y gestos cuando desaparece la superficie nativa,
  aunque no se haya recibido `onPause` de Android.
- Pruebas locales: **3/3** módulos C++ aislados pasaron nuevamente.
- **Importante:** no hay compilación completa `libaseprite.so` verificada
  ni APK jugable. No confundir fuente corregida con ejecución exitosa.


## Puerta de salida: primera APK real — criterios pendientes

El proyecto **no está próximo a una versión instalable verificada** sólo por
haber agregado código de plataforma. La prioridad sigue siendo lograr esta
secuencia con evidencia de ejecución:

1. **Skia Android AArch64**: compilación correcta de `skia`, `skshaper`
   y `skunicode` desde la rama original `aseprite-m124`.
2. **LAF + motor original**: construcción y enlace auténticos de
   `app-lib`, `laf-os` y `src/main/main.cpp` en `libaseprite.so`.
3. **Arranque real**: SurfaceView carga los recursos y aparece la interfaz
   original, no una pantalla vacía ni un editor alternativo.
4. **Operaciones del editor**: abrir, crear, dibujar, capas, deshacer,
   guardar y recuperar ficheros sin corrupción.
5. **Integración del móvil**: entrada táctil, IME, ciclo de vida Android,
   ultrawide sin distorsión y pruebas de estabilidad/rendimiento en
   hardware ARM64.

**Esta continuación:** se corrigió una aserción del test de parches que
interpretaba `\\n` como texto literal. Se introdujo una única referencia
persistente `androidPrimaryScreen()` compartida por `SystemAndroid` y
`WindowAndroid`, para respetar la identidad de pantalla esperada por LAF;
`WindowAndroid::setMousePosition()` ahora actualiza el cursor interno en
coordenadas físicas usando el escalado de la interfaz. Los nuevos cambios
están subidos pero la compilación integral ARM64 y el arranque real no están
verificados.


## 18 Oct 2026 — synchronised ultrawide touch and original startup diagnostics

- Added `rasterViewport()` as shared source-to-screen centering geometry:
  the same helper is used for Skia RGBA/BGRA presentation and original
  `laf` touch-position translation.
- `laf_event_bridge.cpp` now reads the real `os::Surface` dimensions
  during surface changes and subtracts the actual viewport offsets before
  translating touchscreen coordinates to UI pixels. Coordinates outside
  letterboxing remain outside the drawable surface.
- Expanded `raster_presenter_test.cpp` for centered and tall destination
  surfaces and invalid geometry.
- Avoided an RTTI-dependent downcast for the window registered exclusively
  as `WindowAndroid` in this Android build.
- Added explicit `logcat` start, exit code and thread-teardown messages for
  the authentic `app_main()` entrypoint under tag `AsepriteAndroid`.
- Added source-level preflight checks for matching viewport usage in the
  original-event bridge and renderer.

**Current verification limit:** source commits are pushed, but the new CI
runs and the full ARM64 `libaseprite.so` link have not been confirmed
successful. This is still **not** a functional Android APK.


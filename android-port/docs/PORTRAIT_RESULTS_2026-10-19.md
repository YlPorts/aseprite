# Adaptación de pantalla tras el primer arranque en Android

## Evidencia y alcance

El usuario confirmó que la APK anterior carga y adjuntó capturas de Home,
menús, herramientas, paleta, un documento y Preview. Los controles eran
pequeños y quedaban pegados al recorte o a los bordes del teléfono. Esa
evidencia corresponde a `0.1.0-dev`; no acredita dibujo, guardado interactivo
ni funcionamiento de la APK nueva. No se afirma una prueba física propia.

Rama exclusiva: `android-native-port-wip`; `main` intacta.
Commit de implementación: [`6abec2933480a273fcd5a39ac48282b5cee85b61`](https://github.com/YlPorts/aseprite/commit/6abec2933480a273fcd5a39ac48282b5cee85b61).

## Cambios implementados

- **Orientación:** se retira el bloqueo horizontal de Java y del manifest.
  `fullUser` sigue la orientación elegida en Android. `configChanges`
  conserva Activity, hilo C++, documentos y editor al rotar.
- **Área segura:** FrameLayout reserva los insets de cámara, barras,
  gestos de navegación y teclado, más 8 dp. La SurfaceView ocupa ese
  rectángulo: renderizado y tacto tienen el mismo origen local.
  JNI no vuelve a descontar los márgenes.
- **Escala:** la densidad y el ancho útil eligen píxeles enteros; las
  preferencias antiguas de escritorio no reducen la escala por debajo
  del mínimo táctil. En el caso de prueba 1080×2340/densidad 2,75 resulta
  4× en vertical y 5× en horizontal, frente al 2× anterior. No es una
  medición de la densidad ni de los insets reales del Samsung.
  La altura del teclado no se usa para elegir la escala.
- **Opciones de herramientas:** los mismos widgets de ContextBar se
  colocan en filas manteniendo orden y acciones. El dock predeterminado
  permite usar el ancho completo. La altura se recalcula al cambiar de
  herramienta, con protección frente a reentrada del layout.
  El menú original ya admite varias filas y se conserva.
- **Diálogos:** al abrir o redimensionar una ventana que excede el área
  disponible se usan el View y las barras de desplazamiento originales,
  sin añadir envoltorios repetidos. Las ventanas se recolocan dentro
  del área del editor, también tras rotar o mostrar el teclado.
- **Actualización:** versionCode 2, `0.1.1-portrait-dev`, mismo applicationId
  y certificado que la APK privada anterior. No se crea otra aplicación.

Los cambios en fuentes originales se aplican mediante el overlay validado
sobre sus blobs fijados. No se modifica la revisión de LAF ni la de Skia.
No se sustituyen temas, herramientas, algoritmos ni formatos del motor.

## Compilación y pruebas reales

| Comprobación | Resultado |
| --- | --- |
| LAF Android y SkiaWindowAndroid | **COMPILADO**, NDK r27c, API 26, ARM64 |
| app-lib y ui-lib originales con los overlays | **COMPILADO** |
| libaseprite.so | **COMPILADO**, `--no-undefined`, ELF AArch64, siete JNI definidas |
| Módulos C++ | **PROBADO**, CTest 4/4: tacto, cola, raster y distribución en filas |
| Escala y geometría | **PROBADO**, píxeles uniformes 4×/5×, stride y resto de letterbox; opciones en anchos 96–640 sin superposición |
| Java | **COMPILADO** contra Android SDK 35; política de márgenes y escala **PROBADA** con Java real |
| Java/JNI, CMake y overlays | **PROBADO**, contrato con parámetro de escala, fuentes fijadas, reaplicación y dry-run |
| Motor original con Bionic Android 16 | **PROBADO**, 22 invocaciones nativas de la biblioteca de `6abec2933` |
| Archivos | **PROBADO**, capas/grupos/frames/cels/etiquetas/tilemaps, .ase/.aseprite, spritesheets equivalentes, PNG/GIF/JPEG/WebP |
| Gradle existente | **COMPILADO**, `clean assembleDebug`, 35 tareas, BUILD SUCCESSFUL |
| APK final de esta fase | **PROBADO**, siete JNI, 185 recursos originales, firma v2 y manifest fullUser/versionCode 2 |
| Nueva interfaz vertical en el Galaxy | **PENDIENTE** del ensayo del usuario |

Se ejecutaron pruebas del motor bajo QEMU user-mode con Bionic auténtico;
no se inició un emulador Android gráfico en esta fase. Las pruebas de
geometría no equivalen a probar gestos, teclado o ergonomía en el móvil.

## APK privada y biblioteca

- APK: `Aseprite-original-arm64-6abec2933-vertical.apk`, 10.425.571 bytes.
  SHA-256: `51e7c3b3e0affcfdebbd2a3505b574b88f00c3d19097734726d53f487908a291`.
- Motor local sin strip: 206.797.736 bytes.
  SHA-256: `e1f6782ead108f78f16bf6bfd7c061600f0b9b912cdd1f0e1f43abbe2c3b1a57`.
- Build ID del motor y del ELF empaquetado, comprobados iguales:
  `bcad257de9cdc75eca91b8e37b8c473eb18a1e37`.
- Certificado SHA-256, igual al de la candidata anterior:
  `ab910b7452b2f71fbc4c2028874a3086c890c8986147bc3c768788ae476e8ce5`.

La APK se entrega únicamente como archivo privado para uso personal.
No se sube al repositorio público ni a sus artefactos de Actions.

## GitHub Actions

Para `6abec2933` se lanzó una sola revisión de código:

- [Checks 39264743997](https://github.com/YlPorts/aseprite/actions/runs/39264743997): **success**.
- [Preflight 39264743868](https://github.com/YlPorts/aseprite/actions/runs/39264743868): **success**.
- [Skia, motor y pruebas Android 39264743969](https://github.com/YlPorts/aseprite/actions/runs/39264743969):
  **en curso al preparar este informe**; la compilación local completa y
  el empaquetado privado ya terminaron. No se declara exitosa una ejecución
  que todavía está compilando.

No hay un error C++/CMake/enlace pendiente en la compilación local.
No se cancelaron compilaciones para introducir retoques documentales.

## Archivos de implementación y validación

```text
.github/workflows/android-port-checks.yml
android-port/CMakeLists.txt
android-port/app/build.gradle
android-port/app/src/main/AndroidManifest.xml
android-port/app/src/main/java/com/ylports/aseprite/AsepriteActivity.java
android-port/app/src/main/java/com/ylports/aseprite/ViewportPolicy.java
android-port/native-app/CMakeLists.txt
android-port/platform/laf/os/android/window.cpp
android-port/scripts/patch-aseprite-android-engine.py
android-port/src/android/android_raster_window.cpp
android-port/src/android/android_raster_window.h
android-port/src/android/jni_editor_entry.cpp
android-port/src/android/mobile_layout.h
android-port/tests/ViewportPolicyTest.java
android-port/tests/mobile_layout_test.cpp
android-port/tests/native_cmake_topology_smoke.py
android-port/tests/patch_laf_integration_test.py
android-port/tests/raster_presenter_test.cpp
android-port/tests/verify_android_bootstrap_contract.py
```

Se actualizan README, STATUS, BUILD_ANDROID, PROBAR_ANDROID y los informes
con evidencia del usuario y límites actuales.

## Próxima prioridad

Instalar la actualización conservando los datos y comprobar en vertical
los menús, opciones de herramientas, esquinas, cámara, paleta y diálogos.
Después rotar con un documento abierto y verificar que el dedo continúa
dibujando donde corresponde. [Pasos sin PC](PROBAR_ANDROID.md).

La integración SAF, IME completo, herramientas interactivas y estabilidad
general siguen requiriendo trabajo y pruebas; esta fase no acredita que
el port completo esté terminado.

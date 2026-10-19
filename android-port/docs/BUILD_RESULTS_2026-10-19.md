# Informe verificable — port Android ARM64, 19 de octubre de 2026

## Resultado y límite

**COMPILADO:** el Aseprite C++ original y sus bibliotecas originales se
han enlazado en una `libaseprite.so` auténtica Android AArch64. El enlace
usa `--no-undefined`, y el ELF contiene las siete funciones JNI definidas.
El proyecto Gradle existente produjo una APK de desarrollo privada y firmada.
La candidata ya puede entregarse para un primer ensayo en Android ARM64;
esto no acredita todavía su arranque gráfico. El usuario realizará esa prueba
en su teléfono y no requiere validarla antes en un emulador.

**PROBADO:** el motor Android ejecutado con QEMU user-mode y el cargador y
las bibliotecas Bionic de la imagen oficial Android 16 superó guardado y
reapertura de documentos, conservación de capas/grupos/frames/cels,
etiquetas, tilemaps indexados y equivalencia exacta de spritesheets.
Se verificaron importación PNG, `.ase`, `.aseprite`, GIF, JPEG y WebP.

**PENDIENTE:** no se ha iniciado la Activity ni mostrado la interfaz original
sobre una SurfaceView. No se han probado dibujo táctil, IME completo,
herramientas interactivas, SAF, ciclo de vida o rendimiento en el Galaxy A15.
Esto no es todavía el port completo ni una release funcional.

## Fuente y configuración

- Repositorio: https://github.com/YlPorts/aseprite
- Rama exclusiva: `android-native-port-wip`.
- HEAD remoto inicial comprobado: `f3c8f1ed646059a15235018be0668fd686742119`.
- Código nativo y empaquetado de esta fase: hasta `f0c3e4eb0`.
- LAF fijado: `500768cccc6447e3b7816bedbfcb378a5c5222e5`.
- Submódulo clip fijado: `964847c26261f00c5e5bf9ceab622347b75cf85d0`.
- Skia original `aseprite-m124`: `08a5439a6be726021c1c1905d23ce298a3edc5e4`.
- NDK r27c `27.2.12479018`, API 26, `arm64-v8a`, `c++_static`, PIC, Release.
- GN: Android/arm64, raster CPU, GPU/GL/Graphite desactivados,
  `is_trivial_abi=false`; FreeType/HarfBuzz/ICU y codecs originales.
- Generador original `gen` construido para Linux; no se intenta ejecutar
  un generador ARM64 en el runner x86-64.
- Empaquetado local: Gradle 8.9, AGP 8.7.3, OpenJDK 21, SDK 35.
- No se envió ningún cambio a `main`, ni se cambiaron las revisiones fijadas.

## Bloqueos demostrados y correcciones

1. C++ rechazaba la redeclaración de una variable en el `else` de un
   `if` con inicializador del enrutador táctil. Se corrigió su ámbito.
2. CI carecía de lld/binutils/llvm-ar y de `base/config.h` original generado
   por CMake/NDK. Se corrigieron las herramientas y la generación real.
3. El wrapper `depot_tools/gn` exigía `python3_bin_reldir.txt`; no era un
   fallo compilando Skia. Se usa el GN fijado que obtiene el propio Skia.
4. `aligned_alloc` no está disponible en Bionic API 26: LAF usa ahora
   `posix_memalign` y su liberación original. Se probó con tamaños y
   alineaciones distintas y se compiló el código real con el NDK.
5. Android estaba compilando diálogos X11. Se excluyen y se conserva el
   selector original de Aseprite. El portapapeles original usa su backend
   en memoria; la integración compartida Android sigue pendiente.
6. La GUI original requiere los tipos/funciones de clip: se mantiene
   `LAF_WITH_CLIP=ON` en el motor, sin compilar su implementación XCB.
7. El user agent compilaba campos de distribuciones Linux inexistentes
   para Android. Ahora usa la rama Android.
8. CMake encontraba como NOTFOUND JPEG/FreeType/HarfBuzz/WebP debido a
   la búsqueda dentro del sysroot NDK. Se pasan las rutas reales ARM64
   de Skia, sin habilitar búsquedas que mezclen bibliotecas Linux.
9. Bionic integra pthread en libc: se eliminó el enlace `-lpthread`,
   utilizando el resultado original de `FindThreads` solo al enlazar.
10. Se corrigió recursión virtual en `clientSize()`, se conectó el gestor
    Skia de fuentes Android y sus rutas `/system/fonts` y equivalentes.
11. La conversión táctil que lee la superficie ahora ocurre en el hilo
    original del editor, junto con resize. El proceso comienza en su
    directorio privado para el selector y archivos originales.
12. Las comprobaciones `readelf | grep -q` podían provocar SIGPIPE y
    declarar fallido un enlace válido. Se consume la tabla completa y
    se comprueba que las siete exportaciones sean definidas, no UND.
13. El dry-run inicial del parche headless dependía de una mutación que
    el propio dry-run no realiza. Se validan inserción y migración sin
    escribir, y se añadió la prueba sobre blobs originales limpios.
14. **Fallo de ejecución real:** GN m124 habilita `trivial_abi` por defecto
    con `is_official_build=false`, mientras el LAF fijado consume `sk_sp`
    sin ese atributo. Bajo Bionic Android 16, cargar la paleta llegaba a
    `SkiaColorSpace` con un puntero corrupto. LLDB localizó la llamada
    en `SkColorSpace::isNumericalTransferFn`. Al recompilar Skia con
    `is_trivial_abi=false`, el motor pasó las pruebas reales de archivos.
    El auditor rechaza ahora ABI activado o implícito.

Las bibliotecas GN principales ya incluyen dependencias: se inspeccionaron
sus miembros, incluyendo HarfBuzz en skshaper e ICU/datos en skunicode.
No se añadieron indiscriminadamente todos los `.a` al enlazador.

## Verificación ejecutada

| Fase | Resultado real |
| --- | --- |
| A: Skia/skshaper/skunicode Android ARM64 | COMPILADO localmente; 1.403 acciones de recompilación ABI correcta |
| A: gen host, laf-os, app-lib y dependencias | COMPILADO con fuentes originales |
| A: libaseprite.so | COMPILADO; ELF64 LE ET_DYN AArch64, siete JNI definidas, enlace estricto |
| A: arquitectura de archivos estáticos | PROBADO: 3.151 objetos ELF de 50 archivos originales/Skia, todos AArch64 |
| Módulos del port | PROBADO: CTest 3/3, cola/eventos/tacto/raster |
| Integración | PROBADO: blobs originales de submódulos exactos; dry-run inicial y reaplicación sin cambios |
| ELF negativo | PROBADO: rechazo de x86, JNI ausente o embebido como texto, ABI Skia incompatible |
| Motor Android batch | PROBADO: versión y 22 invocaciones nativas en total |
| Tests CLI originales upstream | PROBADO: version, help, list-layers y list-tags, sin modificar los tests |
| Archivos de prueba originales | PROBADO: 1empty3, groups3abc, tags3x123reps, 2x3tilemap-indexed |
| .ase / spritesheet | PROBADO: mismos frames/duración, capas/hierarquía/blend/opacidad y cels; PNG y JSON equivalentes al reabrir |
| .aseprite / PNG | PROBADO: importación, guardado, reapertura y PNG equivalente |
| GIF/JPEG/WebP | PROBADO: exportación y reapertura con dimensiones/frames esperados; no se exige igualdad de píxeles de formatos con pérdida |
| APK | COMPILADO; contenido ELF/JNI, recursos originales y firma v2 verificados |
| B: JNI/Activity/ventana/primer repintado | PENDIENTE |
| C: herramientas mediante interfaz y controles Android | PENDIENTE |
| D: Android gráfico/ultrawide/IME/ciclo de vida/Galaxy A15 | PENDIENTE |

Las 22 invocaciones nativas usan el `.so` real con un lanzador de prueba
que solo llama a `app_main()`. Las bibliotecas de plataforma vienen del
SDK oficial `system-images;android-36;default;arm64-v8a`, imagen del
22 de julio de 2025; no son stubs del NDK ni bibliotecas Linux.
QEMU ejecuta las instrucciones ARM64 y traduce llamadas al kernel del host;
no inicia Android Framework ni reproduce las restricciones del móvil.
El mismo ensayo se añadió al workflow completo, sin crear otro workflow.

## GitHub Actions inspeccionado

- Las ejecuciones históricas [39154936260](https://github.com/YlPorts/aseprite/actions/runs/39154936260)
  y [39155210862](https://github.com/YlPorts/aseprite/actions/runs/39155210862)
  fallaron preparando SDK con el paquete retirado `tools`; no compilaron Skia.
  Se comprobaron las correcciones de SDK ya presentes en la rama inicial.
- La cola conservaba muchos trabajos antiguos de la matriz desktop lanzados
  por commits Android anteriores. Se despejó solo esa cola de la rama Android;
  se mantuvieron los últimos intentos y la concurrencia existente con
  `cancel-in-progress: false`. No se confundió cancelación con error C++.
- [39179927746](https://github.com/YlPorts/aseprite/actions/runs/39179927746)
  completó Skia 1.427/1.427 y las tres bibliotecas AArch64; después falló
  compilando el LAF original en `aligned_alloc`. Ese error está corregido.
- [39182531670](https://github.com/YlPorts/aseprite/actions/runs/39182531670)
  se canceló explícitamente tras demostrar el ABI incompatible, para que
  empezara la revisión corregida pendiente. No fue un fallo del compilador.
- [39182531668](https://github.com/YlPorts/aseprite/actions/runs/39182531668)
  falló en el dry-run inicial, antes de C++. Corregido en `d721c2334`.
- Revisión corregida `d08c3ab69`: [checks 39183119565](https://github.com/YlPorts/aseprite/actions/runs/39183119565),
  [preflight 39183119566](https://github.com/YlPorts/aseprite/actions/runs/39183119566)
  y [core 39183119572](https://github.com/YlPorts/aseprite/actions/runs/39183119572)
  terminaron **success**. [Skia/motor 39183119542](https://github.com/YlPorts/aseprite/actions/runs/39183119542)
  también terminó **success**, con enlace y validación ELF reales.
- Revisión `da3f22070040caaef4cf60039647c787bd10c4a8`:
  [Skia/motor y archivos Android 39184312685](https://github.com/YlPorts/aseprite/actions/runs/39184312685)
  terminó **success**. Se inspeccionó el registro del job `117844338907`:
  `app-lib` completo, enlace estricto, ELF AArch64 con siete JNI definidas
  y las 22 invocaciones del motor real con Bionic Android 16 superadas.
  SHA-256 de la biblioteca generada en ese runner:
  `0e43081de26112093c8e04d2ff7c1c310e18ea63198b85dd7f5ef624f4e32c32`.
  Es una compilación independiente de la local, con otra versión y rutas
  de depuración; no se presenta su hash como el del motor local empaquetado.
  [Checks 39184312725](https://github.com/YlPorts/aseprite/actions/runs/39184312725)
  y [preflight 39184312678](https://github.com/YlPorts/aseprite/actions/runs/39184312678)
  también terminaron **success**.
- El nuevo paso de ejecución batch y el empaquetado privado están en el
  [workflow existente](../../.github/workflows/android-skia-arm64-experimental.yml).
  El empaquetado privado no se ejecuta en este repositorio público.

## Archivos modificados

Se versionan los adaptadores, scripts y pruebas. Los cambios generados
sobre CMake/fuentes originales y submódulos se aplican durante el build;
no se publica una revisión modificada del submódulo ni un binario.

- `.github/workflows/android-integration-preflight.yml`
- `.github/workflows/android-port-checks.yml`
- `.github/workflows/android-skia-arm64-experimental.yml`
- `android-port/.gitignore`
- `android-port/app/build.gradle`
- `android-port/app/src/main/AndroidManifest.xml`
- `android-port/platform/laf/os/android/window.cpp`
- `android-port/scripts/audit-original-skia-arm64.sh`
- `android-port/scripts/patch-aseprite-android-engine.py`
- `android-port/scripts/patch-laf-android-runtime.py`
- `android-port/scripts/patch-laf-headless-android.py`
- `android-port/scripts/stage-authentic-android-assets.py`
- `android-port/scripts/test-original-editor-android-batch.py`
- `android-port/scripts/try-build-original-editor-arm64.sh`
- `android-port/scripts/verify-original-android-apk.py`
- `android-port/src/android/jni_editor_entry.cpp`
- `android-port/src/android/touch_router.cpp`
- `android-port/tests/android_aligned_alloc_test.cpp`
- `android-port/tests/original_editor_cli_main.cpp`
- `android-port/tests/patch_laf_integration_test.py`
- `android-port/tests/test-skia-archive-arm64.sh`
- `android-port/tests/test_asset_stage.py`
- `android-port/README.md`
- `android-port/docs/STATUS.md`
- `android-port/docs/BUILD_ANDROID.md`
- `android-port/docs/PROBAR_ANDROID.md`
- Este informe.

## Commits de implementación de esta continuación

```text
bcf73b9f8 fix(android): compile touch release path without redeclaring condition variable
d13a7cbdf ci(android): install ELF tools and generate genuine NDK headers before preflight
436d6acad ci(android): invoke Skia pinned GN binary without depot_tools bootstrap
48c84b256 fix(android): break original SkiaWindow size query recursion
41912b723 fix(android): use POSIX aligned allocation in pinned LAF at API 26
7d24a0c22 ci(android): provide llvm-ar required by archive regression checks
626eb6a85 fix(android): keep original GUI clipboard API and exclude desktop file dialogs
c5c175cbe fix(android): avoid Linux distro fields in original user agent
b20a198e3 test(android): apply overlays to pristine blobs from exact pinned submodules
2137dde1e fix(android): resolve genuine Skia dependencies and link Bionic threads correctly
6ad4daa1f fix(android): verify defined ELF exports without early-exit pipeline failures
a840666e6 fix(android): keep thread linker flags out of public compiler options
b5953846d fix(android): use native Skia font manager and Android system font paths
c2044f591 fix(android): serialize Skia input mapping with resize and use private working directory
c65211a52 build(android): package verified original engine for private personal-use Actions only
d721c2334 fix(android): validate pristine headless dry run before overlay migration
d08c3ab69 fix(android): match original LAF and Skia smart-pointer calling conventions
cdbff44a2 test(android): execute original engine file roundtrips with genuine Android 16 Bionic
f0c3e4eb0 build(android): use original Aseprite artwork as the launcher icon
da3f22070 docs(android): record genuine ARM64 engine and file tests with private APK build procedure
```

El commit de documentación registra estas evidencias y el procedimiento
privado. No hay commits vacíos ni artefactos binarios versionados.

## Artefactos locales y obtención privada

Artefactos obtenidos tras el empaquetado limpio, motor construido con el código
hasta `f0c3e4eb0` y los overlays descritos (la versión incluye `-dirty` por
esos parches aplicados a los submódulos originales):

- `build/aseprite-gui-android-arm64/lib/libaseprite.so` — 206681824 bytes; SHA-256 `f6eb0f6db88dd132545d07c9d05f32a34aee5857377a27a5c58146b2f18fb954`.
- `android-port/app/build/outputs/apk/debug/app-debug.apk` — 10420075 bytes; SHA-256 `c32b76c6b94eeac87582ceb23ac51ccf708d7b33378d859e88095977525663cc`.
Son resultados de desarrollo; no acreditan ejecución de la APK.
El archivo sin strip contiene información de depuración; Gradle elimina
esa información al empaquetar la biblioteca, conservando el motor.
No se subió una APK ni una biblioteca Aseprite a artefactos públicos.

[Procedimiento para compilar en GitHub Actions desde el navegador del móvil](BUILD_ANDROID.md).
El workflow privado conserva la APK solo tres días y la marca
`RUNTIME-UNVERIFIED`; la firma es de depuración.
La compilación se hace en los servidores de GitHub, no en el teléfono.
La candidata local se prepara aparte para entrega privada, sin subirla al
repositorio público ni a sus artefactos. [Ensayo en el teléfono](PROBAR_ANDROID.md).

## Primer bloqueo pendiente y próxima prioridad

No queda un error C++/CMake/enlace pendiente en la compilación local completa.
La compilación completa y las pruebas batch también terminaron en Actions.
El siguiente resultado pendiente es **ejecutar esta APK y comprobar la
primera interfaz original en Android**.

El frontend del emulador oficial 37.2.12 rechazó ARM64 en este host x86-64:

```text
Avd's CPU Architecture 'arm64' is not supported by the QEMU2 emulator on x86_64 host.
System image must match host architecture.
```

Al usar directamente su backend QEMU con la máquina ARM virtual se llegó
al kernel Android 16 y ADB confirmó ARM64/API 36. ART/zygote falló antes
de habilitar el servicio de paquetes: no se instaló ni inició la APK y no
se cuenta como una prueba de Aseprite. Se detuvieron esos intentos; el
usuario indicó que probará la candidata en su Galaxy A15. No se dispone
físicamente de ese dispositivo. Las pruebas user-mode solo cubren el motor.

Próxima prioridad: instalar la APK privada en Android ARM64,
verificar primera ventana/repintado original y después crear/dibujar/guardar
un documento desde la interfaz. Luego completar SAF, IME (composición,
selección y edición), portapapeles del sistema, controles externos y ciclo
de vida, y medir fluidez/RAM/batería en el A15. Scripting, websocket,
noticias, actualizador e idiomas adicionales están desactivados en esta
configuración de primer arranque; no se afirma cobertura funcional completa.

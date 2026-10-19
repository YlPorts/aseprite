# Compilación del Aseprite original para Android ARM64

Rama: `android-native-port-wip`. No se modifica `main`.
El motor original **ya se enlazó localmente y en GitHub Actions** como `libaseprite.so`, y el
proyecto Gradle existente produjo una APK de desarrollo firmada.
El usuario confirmó el arranque de la primera APK con capturas de la interfaz
y un documento visible. La edición completa y la nueva adaptación vertical
siguen pendientes de comprobación en el teléfono.
Consulta [STATUS.md](STATUS.md) y el [informe verificable](BUILD_RESULTS_2026-10-19.md).

## Compilar en GitHub Actions y descargar desde el móvil

La compilación ocurre en los servidores de GitHub. El móvil solo necesita
su navegador para iniciar el workflow y descargar la APK; no instala NDK,
Gradle ni herramientas de compilación. Si ya tienes la candidata privada,
puedes pasar directamente al [ensayo en Android](PROBAR_ANDROID.md).

Este repositorio es público. Su workflow público compila y verifica el motor,
pero **no sube bibliotecas ni APKs**. Para uso personal, respetando la licencia
original de Aseprite:

1. En GitHub, crea un repositorio **privado independiente** mediante
   [Import a repository](https://github.com/new/import), usando como origen
   `https://github.com/YlPorts/aseprite`. Un fork público no puede hacerse
   privado independientemente de su red de forks.
2. Confirma que la rama `android-native-port-wip` se importó completa, y
   establécela como rama predeterminada **del nuevo repositorio privado**.
   Esto permite mostrar su `workflow_dispatch` en la pestaña Actions.
3. Habilita Actions en ese repositorio privado. Abre **Android ARM64 original
   Skia and Aseprite link (experimental)** y pulsa **Run workflow** sobre
   `android-native-port-wip`, activando `build_private_apk`.
4. Espera al resultado completo. Se compilan Skia, el generador Linux,
   LAF, las bibliotecas originales requeridas y `libaseprite.so`.
   Después se verifica el ELF y se ejecutan pruebas nativas de archivos con
   QEMU y las bibliotecas reales de Android 16. Finalmente se copian los
   recursos originales, se ejecuta Gradle y se inspeccionan el contenido y
   la firma de la APK.
5. Descarga el artefacto privado
   `aseprite-arm64-development-RUNTIME-UNVERIFIED` antes de que expire
   (3 días). Contiene `app-debug.apk`, para pruebas personales.

Activar esa opción en un repositorio público falla antes de compilar.
No se crea una release ni se publica una APK. La firma es de depuración;
una nueva ejecución en otro runner puede usar otra clave, por lo que no
se garantiza actualizar una instalación anterior conservando sus datos.
Respalda tus documentos antes de sustituir una instalación.

## Requisitos y revisiones

- Checkout recursivo de esta rama: `git submodule update --init --recursive`.
- LAF original: `500768cccc6447e3b7816bedbfcb378a5c5222e5`.
- Skia original, rama `aseprite-m124`:
  `08a5439a6be726021c1c1905d23ce298a3edc5e4`.
- NDK r27c `27.2.12479018`, ABI `arm64-v8a`, API mínima 26.
- CMake 3.20 o posterior, Ninja, Python 3, compilador C++ para Linux y
  cabeceras X11 requeridas al configurar el generador original en el host.
- Para empaquetar: JDK 17 o 21, Gradle 8.9, SDK Android 35 y Build Tools 35.

El [workflow existente](../../.github/workflows/android-skia-arm64-experimental.yml)
contiene los comandos completos para sincronizar dependencias y generar GN.
Usa `build/skia/bin/gn`, obtenido por los scripts de la revisión original,
sin pasar por el wrapper de `depot_tools`.

La configuración es `target_os="android"`, `target_cpu="arm64"`,
`ndk_api=26`, **`is_trivial_abi=false`**, raster CPU, GPU/GL/Graphite desactivados, y dependencias
originales de Skia, incluidos FreeType, HarfBuzz y Unicode. Construye:

```sh
ninja -C build/skia/out/Release-Android-arm64 -j 2 skia skshaper skunicode
export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/27.2.12479018"
bash android-port/scripts/audit-original-skia-arm64.sh build/skia/out/Release-Android-arm64
bash android-port/scripts/try-build-original-editor-arm64.sh
```

El script aplica parches comprobados sobre las revisiones fijadas, construye
el `gen` original como ejecutable Linux y después `laf-os`, `app-lib` y
`aseprite` con el NDK. El resultado es:

```text
build/aseprite-gui-android-arm64/lib/libaseprite.so
```

Se usa `c++_static`, PIC y `--no-undefined`. El enlace debe resolver los
símbolos del motor. La comprobación posterior lee ELF64/ET_DYN/AArch64,
las siete exportaciones JNI **definidas** y las dependencias dinámicas.
No acepta que un nombre JNI aparezca solamente como una cadena de texto.

Los archivos GN estáticos principales ya incorporan sus dependencias.
El ABI de `sk_sp` debe coincidir con el LAF original: GN m124 activa
`trivial_abi` implícitamente en builds no oficiales. Desactivarlo
explícitamente evita corrupción de punteros observada al cargar la paleta.
El auditor rechaza tanto `true` como la ausencia de esa opción.
No añadas indiscriminadamente todas las bibliotecas `.a`: la configuración
de Aseprite recibe las rutas Android reales de JPEG, FreeType, HarfBuzz y
WebP. Nunca uses los binarios precompilados Linux x86-64 para Android.

## Empaquetado local del proyecto existente

Solo después del enlace auténtico:

```sh
python3 android-port/scripts/stage-authentic-android-assets.py --engine build/aseprite-gui-android-arm64/lib/libaseprite.so
gradle -p android-port --no-daemon --max-workers 2 clean assembleDebug
python3 android-port/scripts/verify-original-android-apk.py android-port/app/build/outputs/apk/debug/app-debug.apk
"$ANDROID_HOME/build-tools/35.0.0/apksigner" verify android-port/app/build/outputs/apk/debug/app-debug.apk
```

La biblioteca y `data/` se copian a los directorios del módulo existente.
Los recursos generados y los binarios están excluidos de Git. Gradle falla
si falta el motor o `data/gui.xml`. La Activity carga directamente esa
biblioteca y llama al `app_main()` original.

## Pruebas y límites de esta compilación

```sh
bash android-port/scripts/test-host.sh
python3 android-port/tests/patch_laf_integration_test.py
python3 android-port/tests/verify_android_bootstrap_contract.py
python3 android-port/tests/native_cmake_topology_smoke.py
python3 android-port/tests/test_asset_stage.py
bash android-port/tests/test-skia-archive-arm64.sh
```

Los tests ELF requieren Clang, lld y herramientas ELF; los tests de archivos
estáticos también necesitan `llvm-ar`. Sus bibliotecas sintéticas son
fixtures temporales, nunca el motor ni una APK.

Esta configuración de primer arranque desactiva scripting, websocket,
noticias, actualizador e idiomas adicionales. El port todavía requiere
pruebas de interfaz, herramientas y formatos originales; integración SAF;
composición/selección IME completas; portapapeles Android; controles externos;
pausa y restauración; y medidas de rendimiento. No equivale al port completo.
La ventana actual es una sola SurfaceView; Skia raster trabaja en CPU.

El workflow instala la imagen oficial `system-images;android-36;default;arm64-v8a`
y ejecuta el motor con su cargador Bionic bajo QEMU user-mode. Se usa un
lanzador de prueba que solo llama al `app_main()` de la biblioteca real.
Las pruebas verifican guardar/reabrir `.ase` y `.aseprite`, capas/grupos,
frames/cels/etiquetas, tilemaps indexados, spritesheets idénticas y los codecs
PNG/GIF/JPEG/WebP. No inicia Android Framework, JNI ni una SurfaceView; no
sustituye al emulador gráfico o a las pruebas del teléfono.

Para repetirlas en Linux con `qemu-user`, `e2fsprogs`, el NDK y esa imagen:

```sh
python3 android-port/scripts/test-original-editor-android-batch.py --engine build/aseprite-gui-android-arm64/lib/libaseprite.so --system-image "$ANDROID_HOME/system-images/android-36/default/arm64-v8a/system.img"
```

Para el primer ensayo real, instala solamente la APK privada y sigue
[PROBAR_ANDROID.md](PROBAR_ANDROID.md). No requiere PC ni emulador.
Si hay acceso a ADB, el filtro opcional es
`adb logcat -s AsepriteAndroid AndroidRuntime libc DEBUG`.
Las comprobaciones físicas del Galaxy A15 permanecen pendientes.

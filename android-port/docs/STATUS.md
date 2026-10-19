# Estado del port nativo Android — 19 de octubre de 2026

Repositorio original conservado, rama `android-native-port-wip`.
Referencia inicial inspeccionada: `f3c8f1ed646059a15235018be0668fd686742119`.
`main` permanece intacta.

| Componente | Estado real |
| --- | --- |
| Skia original m124: skia, skshaper, skunicode | **COMPILADO** Android ARM64 localmente y en Actions |
| Generador original gen | **COMPILADO** para el runner Linux |
| laf-os, bibliotecas originales y app-lib | **COMPILADO** Android ARM64 localmente y en Actions |
| libaseprite.so original, --no-undefined | **COMPILADO**, ELF AArch64 y siete exportaciones JNI **PROBADAS** |
| Proyecto Gradle existente y recursos originales | **COMPILADO** como APK privada local, firma y contenido **PROBADOS** |
| Motor Android en modo batch: archivos/capas/frames y spritesheets | **PROBADO** localmente y en Actions, bajo QEMU con Bionic Android 16; sin interfaz |
| Primera interfaz original y documento visible en Android | **PROBADO POR EL USUARIO**: capturas de Home y Sprite-0001 |
| Dibujo y guardado desde la GUI | **PENDIENTE** de comprobación completa |
| Márgenes seguros, escala táctil y orientación vertical | **IMPLEMENTADO y COMPILADO**; geometría/paquete **PROBADOS**, ergonomía pendiente en el Galaxy |
| Galaxy A15, Android 16, táctil/IME/ciclo de vida/rendimiento | **PENDIENTE** de pruebas físicas |

El usuario confirmó que la primera candidata carga y envió capturas de
la interfaz original con un documento visible. Detectó controles pequeños
y bordes inaccesibles. Esa evidencia acredita el primer arranque de esa
APK, no el funcionamiento de todas las herramientas ni de la siguiente APK.
No necesita compilar en el teléfono ni realizar pruebas en un emulador.

## Implementación conservada

La Activity y el JNI existentes invocan el app_main() original con
app-lib, LAF y Skia. Se mantienen menús, widgets, herramientas, formatos,
capas y animación del código C++; no hay un editor alternativo.

Hay adaptadores experimentales para SurfaceView/ANativeWindow, raster
RGBA/BGRA, escala entera y stride, cola LAF sincronizada, teclado, presión,
lápiz, tacto/pan/pinch y puente IME solicitado por los campos originales.
Las pruebas de módulos y compilación no certifican esos flujos completos.

## Correcciones de esta fase

- Error C++ real en liberación de tacto; herramientas ELF ausentes en CI.
- GN original de Skia sin el bootstrap roto de depot_tools.
- ABI de Skia alineado con LAF: `is_trivial_abi=false`, tras reproducir
  y depurar corrupción de `sk_sp` al cargar la paleta original.
- posix_memalign compatible con API 26 en el LAF fijado.
- Exclusión de diálogos X11 y selección del backend original de
  portapapeles en memoria; el portapapeles compartido Android falta.
- User agent Android sin campos exclusivos de distribuciones Linux.
- Rutas exactas de JPEG/FreeType/HarfBuzz/WebP de Skia ARM64.
- Hilos Bionic sin intentar enlazar una biblioteca Linux libpthread.
- Recursión en tamaño de ventana corregida; gestor Skia de fuentes Android.
- Conversión de coordenadas táctiles en el hilo del editor para evitar
  competir con la recreación de la superficie.
- Directorio de trabajo privado para el selector original de archivos.
- Validador ELF sin falsos fallos SIGPIPE y empaquetado privado opcional
  dentro del workflow existente, con prohibición de artefactos públicos.

## Próxima prioridad

Entregar y verificar en el Galaxy la adaptación vertical: controles más
grandes, márgenes para cámara/gestos/barras del sistema y opciones originales
en filas. La orientación sigue las preferencias de Android sin reiniciar
el editor al rotar. Se conserva el mismo editor C++, Skia y sus píxeles enteros.

El [workflow completo 39184312685](https://github.com/YlPorts/aseprite/actions/runs/39184312685)
terminó **success** para `da3f22070`: Skia, motor original, verificación ELF
y 22 invocaciones nativas de archivos. No se publicaron binarios.
[La fase vertical](PORTRAIT_RESULTS_2026-10-19.md) registra la nueva APK
`0.1.1-portrait-dev`, su hash, los cambios y sus workflows.
[Pasos del ensayo en el teléfono](PROBAR_ANDROID.md).

Después: crear/dibujar/guardar/reabrir .aseprite con capas y frames;
SAF, IME completo, ratón/teclado externos, pausa/restauración y mediciones
en el Galaxy A15. Scripting y otras opciones de escritorio desactivadas
para esta fase también requieren una revisión posterior.

El [informe de compilación](BUILD_RESULTS_2026-10-19.md) contiene commits,
registros y límites; [BUILD_ANDROID.md](BUILD_ANDROID.md) explica cómo
obtener una APK personal desde Actions en un repositorio privado.

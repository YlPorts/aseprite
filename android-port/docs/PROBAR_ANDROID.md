# Primera prueba de la candidata Android ARM64

Esta APK contiene el editor C++ original, LAF y Skia, con sus recursos
originales. Ya pasó la compilación, la inspección ELF/JNI, la firma y las
pruebas nativas de archivos. **El primer arranque gráfico sigue pendiente.**
Es una candidata de desarrollo para uso personal, no la versión final.

## Instalar sin PC ni emulador

1. Descarga la APK proporcionada de forma privada y ábrela desde Descargas.
2. Si Android lo solicita, permite instalar aplicaciones a ese navegador
   o gestor de archivos y confirma la instalación.
3. Abre **Aseprite** y coloca el Galaxy en horizontal. El primer inicio
   debe copiar los recursos privados y abrir directamente el editor original.

No se compila nada en el móvil. Para generar nuevas versiones por tu cuenta,
usa el [workflow en un repositorio privado](BUILD_ANDROID.md).

La candidata local del 19 de octubre de 2026 tiene 10.420.075 bytes y SHA-256:

```text
c32b76c6b94eeac87582ceb23ac51ccf708d7b33378d859e88095977525663cc
```

## Comprobar por orden

1. **Arranque:** aparecen menús, herramientas, paneles y la interfaz original.
   Una pantalla vacía o un cierre no cuentan como resultado correcto.
2. **Documento:** crea un sprite pequeño, dibuja con el lápiz y prueba
   deshacer/rehacer. Comprueba que el trazo coincide con el dedo.
3. **Interfaz:** abre y cierra menús; prueba zoom/desplazamiento con dos dedos.
4. **Contenido:** añade una capa y un frame, y dibuja algo distinto en ambos.
5. **Archivo:** guarda como `prueba.aseprite` con el selector original,
   cierra el documento, vuelve a abrirlo y verifica dibujo, capas y frames.
6. **Android:** cambia a otra aplicación y vuelve. Comprueba que se conserva
   el documento y que el teclado aparece al editar un campo, sin abrirse al dibujar.

El selector actual usa el almacenamiento privado de la aplicación.
La integración con el selector de documentos Android/SAF aún falta;
no esperes que esta candidata importe desde Descargas o exporte allí.
Desinstalar o borrar los datos de la aplicación elimina esos documentos.
Las firmas de desarrollo pueden variar entre entornos de compilación.

## Información útil para corregir el siguiente fallo

Indica si instala, si llega a mostrar la interfaz, qué paso fue el primero
que falló y el texto exacto de cualquier mensaje. Una captura de la pantalla
o una grabación breve ayuda a comprobar escala y coordenadas; no necesitas
obtener registros ni usar un PC para esta primera prueba.

Si tienes ADB disponible, el filtro opcional es:

```sh
adb logcat -s AsepriteAndroid AndroidRuntime libc DEBUG
```

El [informe verificable](BUILD_RESULTS_2026-10-19.md) separa lo compilado,
lo probado y lo pendiente: herramientas interactivas, IME completo,
SAF, ciclo de vida, rendimiento y uso estable en el Galaxy A15.

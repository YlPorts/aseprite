# Primera prueba de la candidata Android ARM64

Esta APK contiene el editor C++ original, LAF y Skia, con sus recursos
originales. Ya pasó la compilación, la inspección ELF/JNI, la firma y las
pruebas nativas de archivos. El usuario confirmó el primer arranque de la
candidata anterior mediante capturas; la adaptación vertical necesita otro ensayo.
Es una candidata de desarrollo para uso personal, no la versión final.

## Instalar sin PC ni emulador

1. Descarga la APK proporcionada de forma privada y ábrela desde Descargas.
2. Si Android lo solicita, permite instalar aplicaciones a ese navegador
   o gestor de archivos y confirma la instalación.
3. Instala la actualización sobre la anterior y abre **Aseprite** con el
   Galaxy en vertical. La nueva versión admite ambas orientaciones siguiendo
   la rotación de Android. No borres los datos ni desinstales para actualizar.

No se compila nada en el móvil. Para generar nuevas versiones por tu cuenta,
usa el [workflow en un repositorio privado](BUILD_ANDROID.md).

La versión anterior `0.1.0-dev` tenía SHA-256
`c32b76c6b94eeac87582ceb23ac51ccf708d7b33378d859e88095977525663cc`.
La nueva versión es `0.1.1-portrait-dev` (versionCode 2).
Tiene 10.425.571 bytes y SHA-256
`51e7c3b3e0affcfdebbd2a3505b574b88f00c3d19097734726d53f487908a291`.
Se verificó que su certificado coincide con el de la candidata anterior,
para instalarla como actualización conservando los datos privados.

## Comprobar por orden

1. **Pantalla:** aparecen menús, herramientas, paneles y la interfaz original,
   con controles más grandes y separados de los bordes protegidos del teléfono.
   Una pantalla vacía o un cierre no cuentan como resultado correcto.
2. **Documento:** crea un sprite pequeño, dibuja con el lápiz y prueba
   deshacer/rehacer. Comprueba que el trazo coincide con el dedo.
3. **Interfaz:** pulsa menús, herramientas, paleta y botones cercanos a cada
   borde. Cambia de herramienta y comprueba que sus opciones aparecen en filas.
   Abre Nuevo documento y Preferencias; comprueba el desplazamiento de un
   diálogo que no quepa. Prueba zoom/desplazamiento con dos dedos.
4. **Contenido:** añade una capa y un frame, y dibuja algo distinto en ambos.
5. **Archivo:** guarda como `prueba.aseprite` con el selector original,
   cierra el documento, vuelve a abrirlo y verifica dibujo, capas y frames.
6. **Android:** gira de vertical a horizontal y vuelve, sin perder el documento.
   Cambia a otra aplicación y vuelve. Comprueba que el teclado aparece al editar
   un campo y que el trazo sigue coincidiendo con el dedo al regresar al lienzo.

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
La [actualización vertical](PORTRAIT_RESULTS_2026-10-19.md) contiene los
resultados de la segunda candidata.

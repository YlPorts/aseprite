package org.ylports.aseprite;
final class NativeCore {
    static { System.loadLibrary("aseprite_android"); }
    static native long create(int width, int height);
    static native int destroy(long handle);
    static native int[] info(long handle);
    static native int[] render(long handle);
    static native int begin(long handle);
    static native int line(long handle, int x0, int y0, int x1, int y1, int argb, int size, boolean erase);
    static native boolean command(long handle, int operation, int value);
    static native String layers(long handle);
    static native byte[] save(long handle);
    static native int open(long handle, byte[] bytes);
    private NativeCore() {}
}

package com.ylports.aseprite;

/** Physical safe-area and integer-scale policy; no editor/document scaling. */
final class ViewportPolicy {
    private ViewportPolicy() { }

    static int edgePadding(float density, int protectedInset) {
        return Math.max(0, protectedInset) + Math.max(1, (int)Math.ceil(8 * density));
    }

    static int pixelScale(int surfaceWidth, float density) {
        // Aim for readable original pixel fonts, leaving enough logical width
        // for the original dialogs. Height/IME changes must not shrink the UI.
        int readable = Math.max(1, Math.min(8, Math.round(1.75f * density)));
        int fitsWidth = Math.max(1, surfaceWidth / 216);
        return Math.min(readable, fitsWidth);
    }
}

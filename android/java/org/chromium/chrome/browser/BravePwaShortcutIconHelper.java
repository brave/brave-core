/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.graphics.Bitmap;

import androidx.annotation.VisibleForTesting;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browserservices.intents.BitmapHelper;

/**
 * Helper that bounds the size of a PWA icon before it is base64-encoded into a pinned-shortcut
 * Intent.
 *
 * <p>On Android 12+ the system ShortcutService persists pinned shortcuts as Binary XML, where every
 * string is capped at 65535 bytes. A high-resolution icon's base64 string overflows that cap, the
 * whole package XML write fails atomically, and ALL of Brave's pinned shortcuts get dropped on the
 * next reboot. Chrome avoids this by minting WebAPKs (icons live in APK drawables), a Google-only
 * backend Brave cannot use, so Brave falls back to the legacy webapp-shortcut path that carries the
 * icon in the Intent.
 *
 * <p>Downscaling here keeps the encoded icon under the limit. Only the in-Intent copy shrinks; the
 * home-screen glyph is a separate full-resolution Bitmap and is unaffected. The MAC stays
 * consistent because it is computed over this same encoded string.
 */
@NullMarked
public class BravePwaShortcutIconHelper {
    // Keep the encoded icon comfortably below the 65535-byte Binary XML string
    // limit to leave headroom for the rest of the shortcut's persisted strings.
    @VisibleForTesting static final int MAX_ENCODED_ICON_CHARS = 60000;

    // Floor so the in-app icon stays recognizable even for pathological inputs.
    @VisibleForTesting static final int MIN_ICON_SIZE_PX = 48;

    // Bound on shrink iterations; convergence is fast but guard against edge
    // cases (e.g. incompressible noise) where a single pass is not enough.
    private static final int MAX_ATTEMPTS = 10;

    /**
     * Encodes {@code bitmap} as a base64 PNG string like {@link
     * BitmapHelper#encodeBitmapAsString(Bitmap)}, downscaling first if needed so the result fits
     * within the ShortcutService Binary XML string limit.
     *
     * @param bitmap The icon to encode, may be null.
     * @return the base64-encoded (and possibly downscaled) icon string.
     */
    public static String encodeDownscaledBitmapAsString(@Nullable Bitmap bitmap) {
        if (bitmap == null) return "";

        String encoded = BitmapHelper.encodeBitmapAsString(bitmap);
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        for (int attempt = 0;
                attempt < MAX_ATTEMPTS
                        && encoded.length() > MAX_ENCODED_ICON_CHARS
                        && width > MIN_ICON_SIZE_PX
                        && height > MIN_ICON_SIZE_PX;
                attempt++) {
            // Scale proportionally to the overage (base64 length tracks pixel
            // count) so we converge in a couple of passes, then re-encode.
            double ratio = Math.sqrt((double) MAX_ENCODED_ICON_CHARS / encoded.length());
            width = Math.max(MIN_ICON_SIZE_PX, (int) (width * ratio));
            height = Math.max(MIN_ICON_SIZE_PX, (int) (height * ratio));
            Bitmap scaled = Bitmap.createScaledBitmap(bitmap, width, height, true);
            encoded = BitmapHelper.encodeBitmapAsString(scaled);
        }
        return encoded;
    }
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.graphics.Bitmap;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadow.api.Shadow;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.browserservices.intents.BitmapHelper;

import java.io.IOException;
import java.io.OutputStream;

/**
 * Unit tests for {@link BravePwaShortcutIconHelper#encodeDownscaledBitmapAsString}.
 *
 * <p>Robolectric's default Bitmap shadow does not produce a compressed stream whose size tracks
 * pixel count, so the downscale loop would never engage. These tests install {@link
 * ShadowSizedBitmap}, which models compressed output as a fixed number of bytes per pixel — the
 * real-world relationship the helper relies on — making the behaviour deterministic.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(shadows = {BravePwaShortcutIconHelperTest.ShadowSizedBitmap.class})
public class BravePwaShortcutIconHelperTest {
    // The helper's own contract: it caps the encoded icon at MAX_ENCODED_ICON_CHARS
    // (which sits below the 65535-byte ShortcutService Binary-XML limit) and never
    // shrinks an icon below MIN_ICON_SIZE_PX.
    private static final int MAX_ENCODED_ICON_CHARS =
            BravePwaShortcutIconHelper.MAX_ENCODED_ICON_CHARS;
    private static final int MIN_ICON_SIZE_PX = BravePwaShortcutIconHelper.MIN_ICON_SIZE_PX;

    @Before
    public void setUp() {
        ShadowSizedBitmap.reset();
    }

    @Test
    public void testNullBitmapReturnsEmptyString() {
        assertEquals("", BravePwaShortcutIconHelper.encodeDownscaledBitmapAsString(null));
    }

    @Test
    public void testSmallIconPassesThroughUnchanged() {
        // A 96x96 icon encodes well under the limit, so it must not be scaled and
        // must match the plain encoding byte-for-byte.
        Bitmap icon = Bitmap.createBitmap(96, 96, Bitmap.Config.ARGB_8888);
        String result = BravePwaShortcutIconHelper.encodeDownscaledBitmapAsString(icon);

        assertEquals(BitmapHelper.encodeBitmapAsString(icon), result);
        assertEquals("no downscale should occur", 0, ShadowSizedBitmap.sScaleCalls);
    }

    @Test
    public void testOversizedIconIsDownscaledUnderLimit() {
        // A 512x512 icon at 1 byte/pixel encodes far over the limit; the helper
        // must shrink it until the encoded string fits, without dropping below
        // the floor.
        Bitmap icon = Bitmap.createBitmap(512, 512, Bitmap.Config.ARGB_8888);
        String before = BitmapHelper.encodeBitmapAsString(icon);
        assertTrue(
                "precondition: input must exceed the cap",
                before.length() > MAX_ENCODED_ICON_CHARS);

        String result = BravePwaShortcutIconHelper.encodeDownscaledBitmapAsString(icon);

        assertTrue(
                "encoded icon must fit under the cap", result.length() <= MAX_ENCODED_ICON_CHARS);
        assertTrue("downscale must have happened", ShadowSizedBitmap.sScaleCalls > 0);
        assertTrue(
                "output must stay at/above the floor",
                ShadowSizedBitmap.sLastWidth >= MIN_ICON_SIZE_PX);
        assertTrue("output must be smaller than the input", ShadowSizedBitmap.sLastWidth < 512);
        assertNotEquals(before, result);
    }

    @Test
    public void testPathologicalIconClampsToFloorAndTerminates() {
        // 30 bytes/pixel keeps even a 48x48 icon over the limit, forcing the
        // helper down to the floor. It must stop there (not loop forever) and
        // never produce a sub-floor icon.
        ShadowSizedBitmap.sBytesPerPixel = 30;
        Bitmap icon = Bitmap.createBitmap(256, 256, Bitmap.Config.ARGB_8888);

        String result = BravePwaShortcutIconHelper.encodeDownscaledBitmapAsString(icon);

        assertTrue("must not return empty for a valid bitmap", result.length() > 0);
        assertEquals(
                "must clamp to the floor width", MIN_ICON_SIZE_PX, ShadowSizedBitmap.sLastWidth);
        assertEquals(
                "must clamp to the floor height", MIN_ICON_SIZE_PX, ShadowSizedBitmap.sLastHeight);
    }

    /**
     * Bitmap shadow whose compressed size is {@code width * height * sBytesPerPixel} bytes, so the
     * encoded length tracks pixel count the way an incompressible (e.g. random-noise) PNG icon does
     * in practice. Also records the most recently created bitmap's dimensions and the number of
     * scale operations so tests can assert on the loop's behaviour.
     */
    @Implements(Bitmap.class)
    public static class ShadowSizedBitmap {
        public static int sBytesPerPixel;
        public static int sLastWidth;
        public static int sLastHeight;
        public static int sScaleCalls;

        private int mWidth;
        private int mHeight;

        public static void reset() {
            sBytesPerPixel = 1;
            sLastWidth = 0;
            sLastHeight = 0;
            sScaleCalls = 0;
        }

        @Implementation
        protected static Bitmap createBitmap(int width, int height, Bitmap.Config config) {
            Bitmap bitmap = Shadow.newInstanceOf(Bitmap.class);
            ShadowSizedBitmap shadow = Shadow.extract(bitmap);
            shadow.mWidth = width;
            shadow.mHeight = height;
            sLastWidth = width;
            sLastHeight = height;
            return bitmap;
        }

        @Implementation
        protected static Bitmap createScaledBitmap(
                Bitmap src, int dstWidth, int dstHeight, boolean filter) {
            sScaleCalls++;
            return createBitmap(dstWidth, dstHeight, Bitmap.Config.ARGB_8888);
        }

        @Implementation
        protected int getWidth() {
            return mWidth;
        }

        @Implementation
        protected int getHeight() {
            return mHeight;
        }

        @Implementation
        protected boolean compress(Bitmap.CompressFormat format, int quality, OutputStream stream) {
            int byteCount = mWidth * mHeight * sBytesPerPixel;
            try {
                stream.write(new byte[byteCount]);
            } catch (IOException e) {
                return false;
            }
            return true;
        }
    }
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.utils;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;

import androidx.core.content.ContextCompat;

public class DrawableUtils {
    /**
     * Creates a circular drawable from a resource drawable ID.
     *
     * @param context The Android context
     * @param drawableResId Resource ID of the drawable to make circular
     * @param size The desired width and height of the output drawable in pixels
     * @return A circular BitmapDrawable, or null if the input drawable cannot be loaded
     */
    public static Drawable getCircularDrawable(Context context, int drawableResId, int size) {
        // Load the source drawable
        Drawable drawable = ContextCompat.getDrawable(context, drawableResId);
        if (drawable == null) {
            return null;
        }

        // First convert the drawable to a square bitmap of the desired size
        Bitmap sourceBitmap = drawableToBitmap(drawable, size, size);

        // Create a new bitmap that will hold the circular result
        Bitmap outputBitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(outputBitmap);

        // Set up the paint with a shader that will mask the bitmap to a circle
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setShader(
                new BitmapShader(sourceBitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP));

        // Draw a circle using the bitmap shader
        float radius = size / 2f;
        canvas.drawCircle(radius, radius, radius, paint);

        // Convert back to a drawable and return
        return new BitmapDrawable(context.getResources(), outputBitmap);
    }

    /**
     * Converts a Drawable to a Bitmap of the specified dimensions.
     *
     * @param drawable The source drawable to convert
     * @param width The desired width of the output bitmap
     * @param height The desired height of the output bitmap
     * @return A new Bitmap containing the drawable's contents
     */
    private static Bitmap drawableToBitmap(Drawable drawable, int width, int height) {
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);

        // Set the drawable's bounds to match the canvas
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());

        // Draw the drawable onto the bitmap canvas
        drawable.draw(canvas);

        return bitmap;
    }
}

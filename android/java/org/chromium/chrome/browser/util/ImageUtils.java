/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Shader;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;

public class ImageUtils {
    public static Bitmap topOffset(Bitmap src, int offsetY) {
        Bitmap outputimage = Bitmap.createBitmap(src.getWidth(), src.getHeight() + offsetY, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(outputimage);
        canvas.drawBitmap(src, 0, offsetY, null);
        return outputimage;
    }

    public static int calculateInSampleSize(
            BitmapFactory.Options options, int reqWidth, int reqHeight) {
        // Raw height and width of image
        final int height = options.outHeight;
        final int width = options.outWidth;
        int inSampleSize = 1;

        if (height > reqHeight || width > reqWidth) {

            final int halfHeight = height / 2;
            final int halfWidth = width / 2;

            // Calculate the largest inSampleSize value that is a power of 2 and keeps both
            // height and width larger than the requested height and width.
            while ((halfHeight / inSampleSize) >= reqHeight
                    && (halfWidth / inSampleSize) >= reqWidth) {
                inSampleSize *= 2;
            }
        }

        return inSampleSize;
    }

    public static Bitmap addGradient(Bitmap src) {
        Context context = ContextUtils.getApplicationContext();
        int w = src.getWidth();
        int h = src.getHeight();
        Bitmap result = Bitmap.createBitmap(src,0,0,w,h);
        Canvas canvas = new Canvas(result);

        // Top gradient
        int height;

        if(ConfigurationUtils.isLandscape(context)) {
            height = ((2*h)/3);
        } else {
            height = (h/3);
        }

        Paint topPaint = new Paint();
        LinearGradient topShader = new LinearGradient(0,0,0,height, context.getColor(R.color.black_alpha_50), Color.TRANSPARENT, Shader.TileMode.CLAMP);
        topPaint.setShader(topShader);
        topPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DARKEN));
        canvas.drawRect(0,0,w,height,topPaint);

        //Bottom gradient
        Paint bottomPaint = new Paint();
        LinearGradient bottomShader = new LinearGradient(0,2*(h/3),0,h, Color.TRANSPARENT, context.getColor(R.color.black_alpha_30), Shader.TileMode.CLAMP);
        bottomPaint.setShader(bottomShader);
        bottomPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DARKEN));
        canvas.drawRect(0,2*(h/3),w,h,bottomPaint);

        return result;
    }
}
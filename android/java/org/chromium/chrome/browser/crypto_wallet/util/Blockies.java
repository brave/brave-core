/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;

import androidx.annotation.NonNull;

import java.util.Arrays;
import java.util.Locale;

public class Blockies {
    private static final int SIZE = 8;
    private static final int SCALE = 16;
    private static final long[] RAND_SEED = new long[4];
    private static final String[] COLORS = {
        "#423EEE", "#E2E2FC", "#FE5907", "#FEDED6", "#5F5CF1", "#171553", "#1C1E26", "#E1E2E8"
    };

    @NonNull
    public static Bitmap createIcon(
            @NonNull final String address, final boolean lowerCase, final boolean circular) {
        if (lowerCase) {
            return createIcon(address.toLowerCase(Locale.ENGLISH), circular);
        }

        return createIcon(address, circular);
    }

    @NonNull
    public static Drawable createBackground(String address, final boolean lowerCase) {
        if (lowerCase) {
            address = address.toLowerCase(Locale.ENGLISH);
        }

        seedRand(address);
        String color = createColor();
        createColor(); // skip dark color
        String spotColor = createColor(); // use 3rd vibrant color
        GradientDrawable gd =
                new GradientDrawable(
                        GradientDrawable.Orientation.TOP_BOTTOM,
                        new int[] {Color.parseColor(spotColor), Color.parseColor(color)});
        gd.setCornerRadius(0f);
        return gd;
    }

    private static Bitmap createIcon(@NonNull final String address, final boolean circular) {
        seedRand(address);

        String color = createColor();
        String bgColor = createColor();
        String spotColor = createColor();

        double[] imageData = createImageData();

        return createCanvas(imageData, color, bgColor, spotColor, circular);
    }

    private static Bitmap createCanvas(
            @NonNull final double[] imgData,
            @NonNull final String color,
            @NonNull final String bgColor,
            @NonNull final String spotColor,
            final boolean circular) {
        float width = (int) Math.sqrt(imgData.length);

        float w = width * SCALE;
        float h = width * SCALE;

        Bitmap.Config conf = Bitmap.Config.ARGB_8888;
        Bitmap bmp = Bitmap.createBitmap((int) w, (int) h, conf);
        Canvas canvas = new Canvas(bmp);

        int background = Color.parseColor(bgColor);

        Paint paint = new Paint();
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(background);
        canvas.drawRect(0, 0, w, h, paint);

        int main = Color.parseColor(color);
        int spotColorInt = Color.parseColor(spotColor);

        for (int i = 0; i < imgData.length; i++) {
            if (imgData[i] > 0d) {
                float row = (float) Math.floor(i / width);
                float col = i % width;

                final boolean fillColor = imgData[i] == 1.0d;
                paint = new Paint();
                paint.setColor(fillColor ? main : spotColorInt);

                final int shapeType = (int) Math.floor(rand() * 3);
                switch (shapeType) {
                        // Rectangle shape type.
                    case 0:
                        final float rectSizeMultiplier = (float) (rand() * 2);
                        canvas.drawRect(
                                col * SCALE,
                                row * SCALE,
                                (col * SCALE) + SCALE * rectSizeMultiplier,
                                (row * SCALE) + SCALE * rectSizeMultiplier,
                                paint);
                        break;
                        // Circle shape type.
                    case 1:
                        final float circleSizeMultiplier = (float) rand();
                        canvas.drawCircle(
                                col * SCALE + SCALE / 2f,
                                row * SCALE + SCALE / 2f,
                                (SCALE / 2f) * circleSizeMultiplier,
                                paint);
                        break;
                        // Do not add any shape.
                    default:
                        break;
                }
            }
        }

        return getCroppedBitmap(bmp, circular);
    }

    private static double rand() {
        int t = (int) (RAND_SEED[0] ^ (RAND_SEED[0] << 11));
        RAND_SEED[0] = RAND_SEED[1];
        RAND_SEED[1] = RAND_SEED[2];
        RAND_SEED[2] = RAND_SEED[3];
        RAND_SEED[3] = (RAND_SEED[3] ^ (RAND_SEED[3] >> 19) ^ t ^ (t >> 8));

        double num = RAND_SEED[3];
        double den = (1 << 31);

        return Math.abs(num / den);
    }

    @NonNull
    private static String createColor() {
        return COLORS[(int) Math.floor(rand() * 100) % COLORS.length];
    }

    @NonNull
    private static double[] createImageData() {
        double dataWidth = Math.ceil(SIZE / 2f);
        double mirrorWidth = SIZE - dataWidth;

        double[] data = new double[SIZE * SIZE];
        int dataCount = 0;
        for (int y = 0; y < SIZE; y++) {
            double[] row = new double[(int) dataWidth];
            for (int x = 0; x < dataWidth; x++) {
                row[x] = Math.floor(rand() * 2.3d);
            }
            double[] r = Arrays.copyOfRange(row, 0, (int) mirrorWidth);
            reverse(r);
            row = concat(row, r);

            for (int i = 0; i < row.length; i++) {
                data[dataCount] = row[i];
                dataCount++;
            }
        }

        return data;
    }

    public static double[] concat(double[] a, double[] b) {
        int aLen = a.length;
        int bLen = b.length;
        double[] c = new double[aLen + bLen];
        System.arraycopy(a, 0, c, 0, aLen);
        System.arraycopy(b, 0, c, aLen, bLen);

        return c;
    }

    private static void reverse(@NonNull final double[] data) {
        for (int i = 0; i < data.length / 2; i++) {
            double temp = data[i];
            data[i] = data[data.length - i - 1];
            data[data.length - i - 1] = temp;
        }
    }

    @SuppressLint("SelfAssignment")
    private static void seedRand(@NonNull final String seed) {
        Arrays.fill(RAND_SEED, 0);

        for (int i = 0; i < seed.length(); i++) {
            long test = RAND_SEED[i % 4] << 5;
            if (test > Integer.MAX_VALUE << 1 || test < Integer.MIN_VALUE << 1) {
                test = (int) test;
            }

            long test2 = test - RAND_SEED[i % 4];
            RAND_SEED[i % 4] = (test2 + Character.codePointAt(seed, i));
        }

        for (int i = 0; i < RAND_SEED.length; i++) {
            RAND_SEED[i] = (int) RAND_SEED[i];
        }
    }

    @NonNull
    public static Bitmap getCroppedBitmap(@NonNull final Bitmap bitmap, final boolean circular) {
        Bitmap output =
                Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(output);

        final int color = 0xff424242;
        final Paint paint = new Paint();
        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());

        paint.setAntiAlias(true);
        paint.setFilterBitmap(true);
        canvas.drawARGB(0, 0, 0, 0);
        paint.setColor(color);
        if (circular) {
            canvas.drawCircle(
                    bitmap.getWidth() / 2f, bitmap.getHeight() / 2f, bitmap.getWidth() / 2f, paint);
        } else {
            canvas.drawRoundRect(
                    0,
                    0,
                    bitmap.getWidth(),
                    bitmap.getHeight(),
                    WalletConstants.RECT_SHARP_ROUNDED_CORNERS_DP,
                    WalletConstants.RECT_SHARP_ROUNDED_CORNERS_DP,
                    paint);
        }
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
        canvas.drawBitmap(bitmap, rect, rect, paint);

        return output;
    }
}

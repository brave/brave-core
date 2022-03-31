/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;

import java.util.Arrays;
import java.util.Locale;

public class Blockies {
    private static final int SIZE = 8;
    private static final int SCALE = 16;
    private static long[] randSeed = new long[4];
    private static String[] colors = {"#5B5C63", "#151E9A", "#2197F9", "#1FC3DC", "#086582",
            "#67D4B4", "#AFCE57", "#F0CB44", "#F28A29", "#FC798F", "#C1226E", "#FAB5EE", "#9677EE",
            "#5433B0"};

    public static Bitmap createIcon(String address, boolean lowerCase) {
        if (lowerCase) {
            return createIcon(address.toLowerCase(Locale.getDefault()));
        }

        return createIcon(address);
    }

    private static Bitmap createIcon(String address) {
        seedrand(address);

        String color = createColor();
        String bgColor = createColor();
        String spotColor = createColor();

        double[] imgdata = createImageData();

        return createCanvas(imgdata, color, bgColor, spotColor, SCALE);
    }

    private static Bitmap createCanvas(
            double[] imgData, String color, String bgcolor, String spotcolor, int scale) {
        int width = (int) Math.sqrt(imgData.length);

        int w = width * scale;
        int h = width * scale;

        Bitmap.Config conf = Bitmap.Config.ARGB_8888;
        Bitmap bmp = Bitmap.createBitmap(w, h, conf);
        Canvas canvas = new Canvas(bmp);

        int background = Color.parseColor(bgcolor);

        Paint paint = new Paint();
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(background);
        canvas.drawRect(0, 0, w, h, paint);

        int main = Color.parseColor(color);
        int scolor = Color.parseColor(spotcolor);

        for (int i = 0; i < imgData.length; i++) {
            int row = (int) Math.floor(i / width);
            int col = i % width;
            paint = new Paint();

            paint.setColor((imgData[i] == 1.0d) ? main : scolor);

            if (imgData[i] > 0d) {
                canvas.drawRect(col * scale, row * scale, (col * scale) + scale,
                        (row * scale) + scale, paint);
            }
        }

        return blur(getCroppedBitmap(bmp), 1f, 40);
    }

    private static double rand() {
        int t = (int) (randSeed[0] ^ (randSeed[0] << 11));
        randSeed[0] = randSeed[1];
        randSeed[1] = randSeed[2];
        randSeed[2] = randSeed[3];
        randSeed[3] = (randSeed[3] ^ (randSeed[3] >> 19) ^ t ^ (t >> 8));

        double num = (randSeed[3] >>> 0);
        double den = ((1 << 31) >>> 0);

        return Math.abs(num / den);
    }

    private static String createColor() {
        return colors[(int) Math.floor(rand() * 100) % colors.length];
    }

    private static double[] createImageData() {
        int width = SIZE;
        int height = SIZE;

        double dataWidth = Math.ceil(width / 2);
        double mirrorWidth = width - dataWidth;

        double[] data = new double[SIZE * SIZE];
        int dataCount = 0;
        for (int y = 0; y < height; y++) {
            double[] row = new double[(int) dataWidth];
            for (int x = 0; x < dataWidth; x++) {
                row[x] = Math.floor(rand() * 2.3d);
            }
            double[] r = Arrays.copyOfRange(row, 0, (int) mirrorWidth);
            r = reverse(r);
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

    private static double[] reverse(double[] data) {
        for (int i = 0; i < data.length / 2; i++) {
            double temp = data[i];
            data[i] = data[data.length - i - 1];
            data[data.length - i - 1] = temp;
        }

        return data;
    }

    @SuppressLint("SelfAssignment")
    private static void seedrand(String seed) {
        for (int i = 0; i < randSeed.length; i++) {
            randSeed[i] = 0;
        }
        for (int i = 0; i < seed.length(); i++) {
            long test = randSeed[i % 4] << 5;
            if (test > Integer.MAX_VALUE << 1 || test < Integer.MIN_VALUE << 1) {
                test = (int) test;
            }

            long test2 = test - randSeed[i % 4];
            randSeed[i % 4] = (test2 + Character.codePointAt(seed, i));
        }

        for (int i = 0; i < randSeed.length; i++) {
            randSeed[i] = (int) randSeed[i];
        }
    }

    public static Bitmap getCroppedBitmap(Bitmap bitmap) {
        Bitmap output =
                Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(output);

        final int color = 0xff424242;
        final Paint paint = new Paint();
        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());

        paint.setAntiAlias(true);
        canvas.drawARGB(0, 0, 0, 0);
        paint.setColor(color);
        canvas.drawCircle(
                bitmap.getWidth() / 2, bitmap.getHeight() / 2, bitmap.getWidth() / 2, paint);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
        canvas.drawBitmap(bitmap, rect, rect, paint);

        return output;
    }

    private static Bitmap blur(Bitmap sentBitmap, float scale, int radius) {
        int width = Math.round(sentBitmap.getWidth() * scale);
        int height = Math.round(sentBitmap.getHeight() * scale);
        sentBitmap = Bitmap.createScaledBitmap(sentBitmap, width, height, false);

        Bitmap bitmap = sentBitmap.copy(sentBitmap.getConfig(), true);

        if (radius < 1) {
            return (null);
        }

        int w = bitmap.getWidth();
        int h = bitmap.getHeight();

        int[] pix = new int[w * h];
        bitmap.getPixels(pix, 0, w, 0, 0, w, h);

        int wm = w - 1;
        int hm = h - 1;
        int wh = w * h;
        int div = radius + radius + 1;

        int r[] = new int[wh];
        int g[] = new int[wh];
        int b[] = new int[wh];
        int rsum;
        int gsum;
        int bsum;
        int x;
        int y;
        int i;
        int p;
        int yp;
        int yi;
        int yw;
        int vmin[] = new int[Math.max(w, h)];

        int divsum = (div + 1) >> 1;
        divsum *= divsum;
        int dv[] = new int[256 * divsum];
        for (i = 0; i < 256 * divsum; i++) {
            dv[i] = (i / divsum);
        }

        yw = yi = 0;

        int[][] stack = new int[div][3];
        int stackpointer;
        int stackstart;
        int[] sir;
        int rbs;
        int r1 = radius + 1;
        int routsum;
        int goutsum;
        int boutsum;
        int rinsum;
        int ginsum;
        int binsum;

        for (y = 0; y < h; y++) {
            rinsum = ginsum = binsum = routsum = goutsum = boutsum = rsum = gsum = bsum = 0;
            for (i = -radius; i <= radius; i++) {
                p = pix[yi + Math.min(wm, Math.max(i, 0))];
                sir = stack[i + radius];
                sir[0] = (p & 0xff0000) >> 16;
                sir[1] = (p & 0x00ff00) >> 8;
                sir[2] = (p & 0x0000ff);
                rbs = r1 - Math.abs(i);
                rsum += sir[0] * rbs;
                gsum += sir[1] * rbs;
                bsum += sir[2] * rbs;
                if (i > 0) {
                    rinsum += sir[0];
                    ginsum += sir[1];
                    binsum += sir[2];
                } else {
                    routsum += sir[0];
                    goutsum += sir[1];
                    boutsum += sir[2];
                }
            }
            stackpointer = radius;

            for (x = 0; x < w; x++) {
                r[yi] = dv[rsum];
                g[yi] = dv[gsum];
                b[yi] = dv[bsum];

                rsum -= routsum;
                gsum -= goutsum;
                bsum -= boutsum;

                stackstart = stackpointer - radius + div;
                sir = stack[stackstart % div];

                routsum -= sir[0];
                goutsum -= sir[1];
                boutsum -= sir[2];

                if (y == 0) {
                    vmin[x] = Math.min(x + radius + 1, wm);
                }
                p = pix[yw + vmin[x]];

                sir[0] = (p & 0xff0000) >> 16;
                sir[1] = (p & 0x00ff00) >> 8;
                sir[2] = (p & 0x0000ff);

                rinsum += sir[0];
                ginsum += sir[1];
                binsum += sir[2];

                rsum += rinsum;
                gsum += ginsum;
                bsum += binsum;

                stackpointer = (stackpointer + 1) % div;
                sir = stack[(stackpointer) % div];

                routsum += sir[0];
                goutsum += sir[1];
                boutsum += sir[2];

                rinsum -= sir[0];
                ginsum -= sir[1];
                binsum -= sir[2];

                yi++;
            }
            yw += w;
        }
        for (x = 0; x < w; x++) {
            rinsum = ginsum = binsum = routsum = goutsum = boutsum = rsum = gsum = bsum = 0;
            yp = -radius * w;
            for (i = -radius; i <= radius; i++) {
                yi = Math.max(0, yp) + x;

                sir = stack[i + radius];

                sir[0] = r[yi];
                sir[1] = g[yi];
                sir[2] = b[yi];

                rbs = r1 - Math.abs(i);

                rsum += r[yi] * rbs;
                gsum += g[yi] * rbs;
                bsum += b[yi] * rbs;

                if (i > 0) {
                    rinsum += sir[0];
                    ginsum += sir[1];
                    binsum += sir[2];
                } else {
                    routsum += sir[0];
                    goutsum += sir[1];
                    boutsum += sir[2];
                }

                if (i < hm) {
                    yp += w;
                }
            }
            yi = x;
            stackpointer = radius;
            for (y = 0; y < h; y++) {
                // Preserve alpha channel: ( 0xff000000 & pix[yi] )
                pix[yi] = (0xff000000 & pix[yi]) | (dv[rsum] << 16) | (dv[gsum] << 8) | dv[bsum];

                rsum -= routsum;
                gsum -= goutsum;
                bsum -= boutsum;

                stackstart = stackpointer - radius + div;
                sir = stack[stackstart % div];

                routsum -= sir[0];
                goutsum -= sir[1];
                boutsum -= sir[2];

                if (x == 0) {
                    vmin[y] = Math.min(y + r1, hm) * w;
                }
                p = x + vmin[y];

                sir[0] = r[p];
                sir[1] = g[p];
                sir[2] = b[p];

                rinsum += sir[0];
                ginsum += sir[1];
                binsum += sir[2];

                rsum += rinsum;
                gsum += ginsum;
                bsum += binsum;

                stackpointer = (stackpointer + 1) % div;
                sir = stack[stackpointer];

                routsum += sir[0];
                goutsum += sir[1];
                boutsum += sir[2];

                rinsum -= sir[0];
                ginsum -= sir[1];
                binsum -= sir[2];

                yi += w;
            }
        }

        bitmap.setPixels(pix, 0, w, 0, 0, w, h);

        return bitmap;
    }
}

/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.custom_layout.popup_window_tooltip;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;

import androidx.annotation.ColorInt;

public class ArrowColorDrawable extends ColorDrawable {
    public static final int LEFT = 0, TOP = 1, RIGHT = 2, BOTTOM = 3, AUTO = 4;

    private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final int mBackgroundColor;
    private Path mPath;
    private final int mDirection;

    ArrowColorDrawable(@ColorInt int foregroundColor, int direction) {
        this.mBackgroundColor = Color.TRANSPARENT;
        this.mPaint.setColor(foregroundColor);
        this.mDirection = direction;
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        super.onBoundsChange(bounds);
        updatePath(bounds);
    }

    private synchronized void updatePath(Rect bounds) {
        mPath = new Path();

        switch (mDirection) {
            case LEFT:
                mPath.moveTo(bounds.width(), bounds.height());
                mPath.lineTo(0, (float) bounds.height() / 2);
                mPath.lineTo(bounds.width(), 0);
                mPath.lineTo(bounds.width(), bounds.height());
                break;
            case TOP:
                mPath.moveTo(0, bounds.height());
                mPath.lineTo((float) bounds.width() / 2, 0);
                mPath.lineTo(bounds.width(), bounds.height());
                mPath.lineTo(0, bounds.height());
                break;
            case RIGHT:
                mPath.moveTo(0, 0);
                mPath.lineTo(bounds.width(), (float) bounds.height() / 2);
                mPath.lineTo(0, bounds.height());
                mPath.lineTo(0, 0);
                break;
            case BOTTOM:
                mPath.moveTo(0, 0);
                mPath.lineTo((float) bounds.width() / 2, bounds.height());
                mPath.lineTo(bounds.width(), 0);
                mPath.lineTo(0, 0);
                break;
        }

        mPath.close();
    }

    @Override
    public void draw(Canvas canvas) {
        canvas.drawColor(mBackgroundColor);
        if (mPath == null) updatePath(getBounds());
        canvas.drawPath(mPath, mPaint);
    }

    @Override
    public void setAlpha(int alpha) {
        mPaint.setAlpha(alpha);
    }

    public void setColor(@ColorInt int color) {
        mPaint.setColor(color);
    }

    @Override
    public void setColorFilter(ColorFilter colorFilter) {
        mPaint.setColorFilter(colorFilter);
    }

    @Override
    public int getOpacity() {
        if (mPaint.getColorFilter() != null) {
            return PixelFormat.TRANSLUCENT;
        }

        switch (mPaint.getColor() >>> 24) {
            case 255:
                return PixelFormat.OPAQUE;
            case 0:
                return PixelFormat.TRANSPARENT;
        }
        return PixelFormat.TRANSLUCENT;
    }
}

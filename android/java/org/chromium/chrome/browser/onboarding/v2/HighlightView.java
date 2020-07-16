/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

public class HighlightView extends FrameLayout {

    private static final int ALPHA_60_PERCENT = 153;

    private Paint eraserPaint;
    private Paint basicPaint;

    private HighlightItem item;

    private boolean shouldShowHighlight = false;

    public HighlightView(Context context, AttributeSet attrs) {
        super(context, attrs);
        PorterDuffXfermode xfermode = new PorterDuffXfermode(PorterDuff.Mode.MULTIPLY);
        basicPaint = new Paint();
        eraserPaint = new Paint();
        eraserPaint.setColor(0xFFFFFF);
        eraserPaint.setAlpha(0);
        eraserPaint.setXfermode(xfermode);
        eraserPaint.setAntiAlias(true);
    }

    public void setHighlightItem(HighlightItem item) {
        this.item = item;
        invalidate();
    }

    public void setShouldShowHighlight(boolean shouldShowHighlight) {
        this.shouldShowHighlight = shouldShowHighlight;
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        int[] location = new int[2];
        getLocationOnScreen(location);
        Bitmap overlay = Bitmap.createBitmap(getMeasuredWidth(), getMeasuredHeight(),
                Bitmap.Config.ARGB_8888);
        Canvas overlayCanvas = new Canvas(overlay);
        overlayCanvas.drawColor(0xcc1E2029);// 0x3333B5E5);
        eraserPaint.setAlpha(ALPHA_60_PERCENT);
        if (shouldShowHighlight) {
            int width = item.getScreenRight() - item.getScreenLeft();
            int height = item.getScreenBottom() - item.getScreenTop();
            int cx = item.getScreenLeft() + width / 2 - location[0];
            int cy = item.getScreenTop() + height / 2 - location[1];
            float radius = width > height ? ((float) width / 2) : ((float) height / 2);
            float outerRadiusScaleMultiplier = 1.2f;
            overlayCanvas.drawCircle(cx, cy, radius * outerRadiusScaleMultiplier, eraserPaint);
            eraserPaint.setAlpha(0);
            float innerRadiusScaleMultiplier = 0.8f;
            overlayCanvas.drawCircle(cx, cy, radius * innerRadiusScaleMultiplier, eraserPaint);
        }
        canvas.drawBitmap(overlay, 0, 0, basicPaint);
        super.dispatchDraw(canvas);
    }
}
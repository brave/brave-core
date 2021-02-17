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
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import org.chromium.chrome.R;
import org.chromium.ui.base.DeviceFormFactor;

public class HighlightView extends FrameLayout {

    private static final int ALPHA_60_PERCENT = 153;

    private Paint eraserPaint;
    private Paint basicPaint;

    private HighlightItem item;

    private boolean shouldShowHighlight;

    private Context context;

    public HighlightView(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
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
        overlayCanvas.drawColor(0xcc1E2029);
        int width = item.getScreenRight() - item.getScreenLeft();
        int height = item.getScreenBottom() - item.getScreenTop();
        float radius = width > height ? ((float) width / 2) : ((float) height / 2);

        Paint innerBorderPaint = new Paint();
        innerBorderPaint.setStyle(Paint.Style.STROKE);
        innerBorderPaint.setStrokeWidth(2); // set stroke width
        innerBorderPaint.setColor(Color.parseColor("#FFFFFF")); // set stroke color
        innerBorderPaint.setAntiAlias(true);

        Paint outterBorderPaint = new Paint();
        outterBorderPaint.setStyle(Paint.Style.STROKE);
        outterBorderPaint.setStrokeWidth(3); // set stroke width
        outterBorderPaint.setColor(Color.parseColor("#FFFFFF")); // set stroke color
        outterBorderPaint.setAntiAlias(true);
        if (shouldShowHighlight) {
            int cx = item.getScreenLeft() + width / 2 - location[0];
            int cy = item.getScreenTop() + height / 2 - location[1];

            eraserPaint.setAlpha(0);
            float innerRadiusScaleMultiplier = 0.8f;
            overlayCanvas.drawCircle(cx, cy, radius * innerRadiusScaleMultiplier, eraserPaint);
            overlayCanvas.drawCircle(cx, cy, radius * innerRadiusScaleMultiplier, innerBorderPaint);
            
            eraserPaint.setAlpha(ALPHA_60_PERCENT);
            float outerRadiusScaleMultiplier = 1.2f;
            overlayCanvas.drawCircle(cx, cy, radius * outerRadiusScaleMultiplier, eraserPaint);
            overlayCanvas.drawCircle(cx, cy, radius * outerRadiusScaleMultiplier, outterBorderPaint);

        } else {
            boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);
            int verticalOffset = isTablet ? 35 : 60;
            eraserPaint.setAlpha(0);
            RectF innerRect = new RectF(item.getScreenLeft() + 10,
                    (item.getScreenTop() - verticalOffset) + 10, item.getScreenRight() - 10,
                    item.getScreenBottom() - (isTablet ? 45 : 10));
            overlayCanvas.drawRoundRect(innerRect, 12, 12, eraserPaint);
            overlayCanvas.drawRoundRect(innerRect, 12, 12, innerBorderPaint);

            eraserPaint.setAlpha(ALPHA_60_PERCENT);
            RectF outerRect = new RectF(item.getScreenLeft(), item.getScreenTop() - verticalOffset,
                    item.getScreenRight(), item.getScreenBottom() - (isTablet ? 35 : 0));
            overlayCanvas.drawRoundRect(outerRect, 22, 22, eraserPaint);
            overlayCanvas.drawRoundRect(outerRect, 22, 22, outterBorderPaint);
        }
        canvas.drawBitmap(overlay, 0, 0, basicPaint);
        super.dispatchDraw(canvas);
    }
}
/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Shader;
import android.util.AttributeSet;
import android.view.View;

import java.util.ArrayList;
import java.util.List;

public class SmoothLineChartEquallySpaced extends View {
    private static final int CIRCLE_SIZE = 8;
    private static final int STROKE_SIZE = 2;
    private static final float SMOOTHNESS = 0.35f; // the higher the smoother, but don't go over 0.5

    private final Paint mPaint;
    private final Path mPath;
    private final float mCircleSize;
    private final float mStrokeSize;
    private final float mBorder;

    private float[] mValues;
    private float mMaxY;

    public SmoothLineChartEquallySpaced(Context context) {
        this(context, null, 0);
    }

    public SmoothLineChartEquallySpaced(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SmoothLineChartEquallySpaced(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        float scale = context.getResources().getDisplayMetrics().density;

        mCircleSize = scale * CIRCLE_SIZE;
        mStrokeSize = scale * STROKE_SIZE;
        mBorder = mCircleSize;

        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStrokeWidth(mStrokeSize);

        mPath = new Path();
    }

    public void setData(float[] values) {
        mValues = values;

        if (values != null && values.length > 0) {
            mMaxY = values[0];
            // mMinY = values[0].y;
            for (float y : values) {
                if (y > mMaxY) mMaxY = y;
                /*if (y < mMinY)
                        mMinY = y;*/
            }
        }

        invalidate();
    }

    public void draw(Canvas canvas) {
        super.draw(canvas);

        if (mValues == null || mValues.length == 0) return;

        int size = mValues.length;

        final float height = getMeasuredHeight() - 2 * mBorder;
        final float width = getMeasuredWidth() - 2 * mBorder;

        final float dX = mValues.length > 1 ? mValues.length - 1 : (2);
        float mMinY = 0;
        final float dY = (mMaxY - mMinY) > 0 ? (mMaxY - mMinY) : (2);

        mPath.reset();

        // calculate point coordinates
        List<PointF> points = new ArrayList<>(size);
        for (int i = 0; i < size; i++) {
            float x = mBorder + i * width / dX;
            float y = mBorder + height - (mValues[i] - mMinY) * height / dY;
            points.add(new PointF(x, y));
        }

        // calculate smooth path
        float lX = 0, lY = 0;
        mPath.moveTo(points.get(0).x, points.get(0).y);
        for (int i = 1; i < size; i++) {
            PointF p = points.get(i); // current point

            // first control point
            PointF p0 = points.get(i - 1); // previous point
            float x1 = p0.x + lX;
            float y1 = p0.y + lY;

            // second control point
            PointF p1 = points.get(i + 1 < size ? i + 1 : i); // next point
            lX = (p1.x - p0.x) / 2 * SMOOTHNESS; // (lX,lY) is the slope of the reference line
            lY = (p1.y - p0.y) / 2 * SMOOTHNESS;
            float x2 = p.x - lX;
            float y2 = p.y - lY;

            // add line
            mPath.cubicTo(x1, y1, x2, y2, p.x, p.y);
        }

        // draw path
        LinearGradient linearGradient = new LinearGradient(0, 0, width, height,
                new int[] {0xFFF73A1C, 0xFFBF14A2,
                        0xFF6F4CD2}, // substitute the correct colors for these
                new float[] {0, 0.60f, 0.90f}, Shader.TileMode.CLAMP);
        mPaint.setShader(linearGradient);
        mPaint.setStyle(Style.STROKE);
        canvas.drawPath(mPath, mPaint);

        // draw circles
        mPaint.setStyle(Style.FILL_AND_STROKE);
        canvas.drawCircle(points.get(points.size() - 1).x, points.get(points.size() - 1).y,
                mCircleSize / 2, mPaint);
        mPaint.setStyle(Style.FILL);
        mPaint.setColor(Color.WHITE);
        canvas.drawCircle(points.get(points.size() - 1).x, points.get(points.size() - 1).y,
                (mCircleSize - mStrokeSize) / 2, mPaint);
    }
}

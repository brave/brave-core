/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Shader;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.chrome.R;

import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class SmoothLineChartEquallySpaced extends View {
    private static final int CIRCLE_SIZE = 8;
    private static final int STROKE_SIZE = 2;
    private static final float SMOOTHNESS = 0.35f; // the higher the smoother, but don't go over 0.5

    private static final int CANVAS_TEXT_START_Y = 35;
    private static final int CANVAS_TEXT_PADDING_Y = 10;

    private final Paint mPaint;
    private final Path mPath;
    private final float mCircleSize;
    private final float mStrokeSize;
    private final float mBorder;

    private float[] mValues;
    private String[] mDates;
    private float mMinY;
    private float mMaxY;
    private float mCurrentLineX;
    private boolean mNoDrawText;
    private int[] colors;
    private TextView mPrice;

    public SmoothLineChartEquallySpaced(Context context) {
        this(context, null, 0);
    }

    public SmoothLineChartEquallySpaced(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SmoothLineChartEquallySpaced(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        float scale = context.getResources().getDisplayMetrics().density;

        mCurrentLineX = -1;
        mCircleSize = scale * CIRCLE_SIZE;
        mStrokeSize = scale * STROKE_SIZE;
        mBorder = mCircleSize;

        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStrokeWidth(mStrokeSize);

        mPath = new Path();
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    public void setData(AssetTimePrice[] data) {
        mValues = new float[data.length];
        mDates = new String[data.length];
        for (int index = 0; index < data.length; index++) {
            mValues[index] = Float.parseFloat(data[index].price);
            Date date = new Date(data[index].date.microseconds / 1000);
            DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm a", Locale.getDefault());
            mDates[index] = dateFormat.format(date);
        }

        if (mValues != null && mValues.length > 0) {
            mMinY = mValues[0];
            mMaxY = mValues[0];
            for (float y : mValues) {
                if (y > mMaxY) mMaxY = y;
                if (y < mMinY) mMinY = y;
            }
        }

        invalidate();
    }

    public void setData(float[] values) {
        mValues = values;
        mDates = new String[values.length];

        if (values != null && values.length > 0) {
            mMinY = values[0];
            mMaxY = values[0];
            for (float y : values) {
                if (y > mMaxY) mMaxY = y;
                if (y < mMinY) mMinY = y;
            }
        }
        for (int index = 0; index < mValues.length; index++) {
            Date date = new Date();
            DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm a", Locale.getDefault());
            mDates[index] = dateFormat.format(date);
        }

        invalidate();
    }

    public void drawLine(float x, TextView price) {
        mNoDrawText = false;
        mPrice = price;
        mCurrentLineX = x;
        invalidate();
    }

    public void setNoDrawText(boolean noDraw) {
        mNoDrawText = noDraw;
    }

    public void setColors(int[] colors) {
        this.colors = colors;
    }

    @Override
    @SuppressLint("SetTextI18n")
    public void draw(Canvas canvas) {
        super.draw(canvas);

        if (mValues == null || mValues.length == 0) return;

        int size = mValues.length;

        final float height = getMeasuredHeight() - 2 * mBorder;
        final float width = getMeasuredWidth() - 2 * mBorder;

        final float dX = mValues.length > 1 ? mValues.length - 1 : (2);
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
        float lX = 0;
        float lY = 0;
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
        if (colors.length > 1) {
            LinearGradient linearGradient = new LinearGradient(0, 0, width, height,
                    colors, // substitute the correct colors for these
                    new float[] {0, 0.60f, 0.90f}, Shader.TileMode.CLAMP);
            mPaint.setShader(linearGradient);
        } else {
            mPaint.setColor(colors[0]);
        }
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

        // Draw vertical lines
        if (mCurrentLineX != -1) {
            Paint paint = new Paint();
            paint.setColor(getContext().getColor(R.color.wallet_text_color));

            paint.setStrokeWidth(2f);
            canvas.drawLine(mCurrentLineX, CANVAS_TEXT_START_Y + CANVAS_TEXT_PADDING_Y,
                    mCurrentLineX, getHeight() - mBorder - mStrokeSize / 2, paint);
            float possibleValue =
                    mValues.length > 1 ? (mCurrentLineX / (width / mValues.length)) : 0;
            if (possibleValue < 0) {
                possibleValue = 0;
            } else if (possibleValue >= mDates.length) {
                possibleValue = mDates.length - 1;
            }
            TextPaint paintText = new TextPaint(TextPaint.ANTI_ALIAS_FLAG);
            paintText.setColor(getContext().getColor(R.color.wallet_text_color));
            paintText.setTextSize(35);
            float textX = mCurrentLineX - 150;
            if (textX < 0) {
                textX = mCurrentLineX;
            }
            if (!mNoDrawText)
                canvas.drawText(String.valueOf(mDates[(int) possibleValue]), textX,
                        CANVAS_TEXT_START_Y, paintText);
            if (mPrice != null) {
                DecimalFormat decimalFormat = new DecimalFormat("#,##0.00");
                mPrice.setText("$" + decimalFormat.format(mValues[(int) possibleValue]));
            }
        }
    }
}

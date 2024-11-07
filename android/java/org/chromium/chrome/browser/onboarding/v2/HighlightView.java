/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.FrameLayout;

import androidx.annotation.Keep;
import androidx.core.content.ContextCompat;

public class HighlightView extends FrameLayout {

    private static final int ALPHA_60_PERCENT = 153;
    private static final int DEFAULT_ANIMATION_DURATION = 1000;

    private Paint mEraserPaint;
    private Paint mBasicPaint;

    private HighlightItem mItem;
    private int mItemWidth;
    private int mItemHeight;

    private boolean mShouldShowHighlight;
    private boolean mIsAnimating;
    private boolean mIsHighlightTransparent;
    private AnimatorSet mAnimatorSet;

    private float mInnerRadius;
    private float mOuterRadius;
    private int mColor = -1;

    public HighlightView(Context context, AttributeSet attrs) {
        super(context, attrs);
        PorterDuffXfermode xfermode = new PorterDuffXfermode(PorterDuff.Mode.MULTIPLY);
        mBasicPaint = new Paint();
        mEraserPaint = new Paint();
        if (mColor == -1) {
            mColor = ContextCompat.getColor(context, android.R.color.white);
        }

        mEraserPaint.setColor(0xFFFFFF);
        mEraserPaint.setAlpha(0);
        mEraserPaint.setXfermode(xfermode);
        mEraserPaint.setAntiAlias(true);
    }

    public void setColor(int color) {
        mColor = color;
        invalidate();
    }

    public void setHighlightItem(HighlightItem item) {
        mItem = item;
        mItemWidth = item.getScreenRight() - item.getScreenLeft();
        mItemHeight = item.getScreenBottom() - item.getScreenTop();

        float radius =
                mItemWidth > mItemHeight ? ((float) mItemWidth / 2) : ((float) mItemHeight / 2);
        setInnerRadius(radius);
        setOuterRadius(radius);
        invalidate();
    }

    public void setShouldShowHighlight(boolean shouldShowHighlight) {
        mShouldShowHighlight = shouldShowHighlight;
    }

    public void setHighlightTransparent(boolean isHighlightTransparent) {
        mIsHighlightTransparent = isHighlightTransparent;
    }

    public void initializeAnimators() {
        mAnimatorSet = new AnimatorSet();

        ObjectAnimator scaleXAnimator = ObjectAnimator.ofFloat(
                this, "innerRadius", mInnerRadius * 0.7f, mInnerRadius * 1.1f);
        scaleXAnimator.setRepeatCount(ValueAnimator.INFINITE);

        ObjectAnimator scaleBigAnimator = ObjectAnimator.ofFloat(
                this, "outerRadius", mOuterRadius * 0.9f, mOuterRadius * 1.2f);

        scaleBigAnimator.setRepeatCount(ValueAnimator.INFINITE);

        mAnimatorSet.setDuration(DEFAULT_ANIMATION_DURATION);
        mAnimatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        mAnimatorSet.playTogether(scaleXAnimator, scaleBigAnimator);
    }

    public void startAnimation() {
        if (mIsAnimating) {
            // already animating
            return;
        }
        mAnimatorSet.start();
        mIsAnimating = true;
    }

    public void stopAnimation() {
        if (!mIsAnimating) {
            // already not animating
            return;
        }
        mAnimatorSet.end();
        mIsAnimating = false;
    }

    @Keep
    public void setInnerRadius(float radius) {
        mInnerRadius = radius;
        invalidate();
    }

    @Keep
    public void setOuterRadius(float radius) {
        mOuterRadius = radius;
        invalidate();
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        int[] location = new int[2];
        getLocationOnScreen(location);
        Bitmap overlay = Bitmap.createBitmap(getMeasuredWidth(), getMeasuredHeight(),
                Bitmap.Config.ARGB_8888);
        Canvas overlayCanvas = new Canvas(overlay);
        overlayCanvas.drawColor(0xB3000000);

        if (mItem == null) {
            return;
        }

        Paint innerBorderPaint = new Paint();
        innerBorderPaint.setStyle(Paint.Style.STROKE);
        innerBorderPaint.setStrokeWidth(2); // set stroke width
        innerBorderPaint.setColor(mColor); // set stroke color
        innerBorderPaint.setAntiAlias(true);

        Paint outterBorderPaint = new Paint();
        outterBorderPaint.setStyle(Paint.Style.STROKE);
        outterBorderPaint.setStrokeWidth(3); // set stroke width
        outterBorderPaint.setColor(mColor); // set stroke color
        outterBorderPaint.setAntiAlias(true);

        if (mShouldShowHighlight) {
            int cx = mItem.getScreenLeft() + mItemWidth / 2 - location[0];
            int cy = mItem.getScreenTop() + mItemHeight / 2 - location[1];

            if (mIsHighlightTransparent) {
                mEraserPaint.setAlpha(255);
            } else {
                mEraserPaint.setAlpha(0);
            }

            float innerRadiusScaleMultiplier = 0.7f;
            overlayCanvas.drawCircle(
                    cx, cy, mInnerRadius * innerRadiusScaleMultiplier, mEraserPaint);
            overlayCanvas.drawCircle(
                    cx, cy, mInnerRadius * innerRadiusScaleMultiplier, innerBorderPaint);

            mEraserPaint.setAlpha(255);
            float outerRadiusScaleMultiplier = 1.2f;
            overlayCanvas.drawCircle(
                    cx, cy, mOuterRadius * outerRadiusScaleMultiplier, mEraserPaint);
            overlayCanvas.drawCircle(
                    cx, cy, mOuterRadius * outerRadiusScaleMultiplier, outterBorderPaint);

        } else {
            if (mIsHighlightTransparent) {
                mEraserPaint.setAlpha(255);
            } else {
                mEraserPaint.setAlpha(0);
            }
            innerBorderPaint.setStrokeWidth(6);
            outterBorderPaint.setStrokeWidth(6);

            RectF innerRect = new RectF(mItem.getScreenLeft() + 10,
                    (mItem.getScreenTop() - location[1]) + 10, mItem.getScreenRight() - 10,
                    mItem.getScreenBottom() - 10 - location[1]);
            overlayCanvas.drawRoundRect(innerRect, 12, 12, mEraserPaint);
            if (mIsHighlightTransparent) {
                overlayCanvas.drawRoundRect(innerRect, 12, 12, innerBorderPaint);
            }

            RectF outerRect = new RectF(mItem.getScreenLeft(), mItem.getScreenTop() - location[1],
                    mItem.getScreenRight(), mItem.getScreenBottom() - location[1]);

            if (!mIsHighlightTransparent) {
                mEraserPaint.setAlpha(ALPHA_60_PERCENT);
            }

            overlayCanvas.drawRoundRect(outerRect, 22, 22, mEraserPaint);
            if (!mIsHighlightTransparent) {
                overlayCanvas.drawRoundRect(outerRect, 22, 22, outterBorderPaint);
            }
        }
        canvas.drawBitmap(overlay, 0, 0, mBasicPaint);
        super.dispatchDraw(canvas);
    }
}

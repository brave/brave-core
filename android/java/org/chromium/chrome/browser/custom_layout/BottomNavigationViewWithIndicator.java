/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.custom_layout;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.MenuItem;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.math.MathUtils;
import androidx.core.view.OneShotPreDrawListener;
import androidx.interpolator.view.animation.LinearOutSlowInInterpolator;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;

import org.chromium.chrome.R;
import org.chromium.ui.base.ViewUtils;

/**
 * Bottom navigation view with a gradient indicator below the selected item.
 */
public class BottomNavigationViewWithIndicator
        extends BottomNavigationView implements NavigationBarView.OnItemSelectedListener {
    private static final float DEFAULT_SCALE = 1f;
    private static final float MAX_SCALE = 2f;

    private static final long BASE_DURATION = 300L;
    private static final long VARIABLE_DURATION = 300L;

    private static final int SEPARATOR_HEIGHT_DP = 4;

    @Nullable
    private OnItemSelectedListener mExternalSelectedListener;
    @Nullable
    private ValueAnimator mAnimator;

    private Drawable mShape;

    private RectF mIndicator;
    private Rect mBounds;

    private float mDefaultSizePx;
    private float mSeparatorHeight;

    public BottomNavigationViewWithIndicator(@NonNull Context context) {
        super(context);
        init();
    }

    public BottomNavigationViewWithIndicator(
            @NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public BottomNavigationViewWithIndicator(
            @NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public BottomNavigationViewWithIndicator(@NonNull Context context, @Nullable AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        super.setOnItemSelectedListener(this);

        mShape = AppCompatResources.getDrawable(
                getContext(), R.drawable.bottom_navigation_gradient_separator);
        mSeparatorHeight = ViewUtils.dpToPx(getContext(), SEPARATOR_HEIGHT_DP);
        mIndicator = new RectF();
        mBounds = new Rect();
        mDefaultSizePx = 0.0f;
    }

    @Override
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {
        if (mExternalSelectedListener != null
                && mExternalSelectedListener.onNavigationItemSelected(item)
                && item.getItemId() != getSelectedItemId()) {
            onItemSelected(item.getItemId(), true);
            return true;
        }
        return false;
    }

    @Override
    public void setOnItemSelectedListener(@Nullable OnItemSelectedListener listener) {
        mExternalSelectedListener = listener;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        OneShotPreDrawListener.add(this, () -> {
            mDefaultSizePx = findViewById(getSelectedItemId()).getWidth();
            // Move the indicator in place when the view is laid out.
            onItemSelected(getSelectedItemId(), false);
        });
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        cancelAnimator(true);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        super.dispatchDraw(canvas);
        if (isAttachedToWindow()) {
            mIndicator.roundOut(mBounds);
            mShape.setBounds(mBounds);
            mShape.draw(canvas);
        }
    }

    private void onItemSelected(int itemId, boolean animate) {
        if (!isAttachedToWindow()) return;

        // Interrupt any current animation, but don't set the end values,
        // if it's in the middle of a movement we want it to start from
        // the current position, to make the transition smoother.
        cancelAnimator(false);

        final View itemView = findViewById(itemId);
        final float fromCenterX = mIndicator.centerX();
        final float fromScale = mIndicator.width() / mDefaultSizePx;

        mAnimator = ValueAnimator.ofFloat(fromScale, MAX_SCALE, DEFAULT_SCALE);
        mAnimator.addUpdateListener(animation -> {
            float progress = animation.getAnimatedFraction();
            float distanceTravelled =
                    linearInterpolation(progress, fromCenterX, getCenterX(itemView));

            float scale = (float) animation.getAnimatedValue();
            float indicatorWidth = mDefaultSizePx * scale;

            float left = distanceTravelled - indicatorWidth / 2f;
            float top = getHeight() - mSeparatorHeight;
            float right = distanceTravelled + indicatorWidth / 2f;
            float bottom = getHeight();

            mIndicator.set(left, top, right, bottom);
            invalidate();
        });

        mAnimator.setInterpolator(new LinearOutSlowInInterpolator());

        if (animate) {
            float distanceToMove = Math.abs(fromCenterX - getCenterX(itemView));
            mAnimator.setDuration(calculateDuration(distanceToMove));
        } else {
            mAnimator.setDuration(0L);
        }

        mAnimator.start();
    }

    /**
     * Linear interpolation between 'a' and 'b' based on the progress 't'.
     */
    private float linearInterpolation(float t, float a, float b) {
        return (1 - t) * a + t * b;
    }

    /**
     * Calculates a duration for the translation based on a fixed duration + a duration
     * based on the distance the indicator is being moved.
     */
    private long calculateDuration(float distance) {
        return (long) (BASE_DURATION
                + VARIABLE_DURATION * MathUtils.clamp(distance / getWidth(), 0f, 1f));
    }

    /**
     * Gets the center X value of a view.
     */
    private float getCenterX(@NonNull final View view) {
        return view.getLeft() + view.getWidth() / 2f;
    }

    private void cancelAnimator(final boolean setEndValues) {
        final ValueAnimator currentAnimator = mAnimator;
        if (currentAnimator == null) return;

        if (setEndValues) {
            currentAnimator.end();
        } else {
            currentAnimator.cancel();
        }
        currentAnimator.removeAllUpdateListeners();
        mAnimator = null;
    }
}

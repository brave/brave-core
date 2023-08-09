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

import org.chromium.chrome.R;
import org.chromium.ui.base.ViewUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.math.MathUtils;
import androidx.core.view.OneShotPreDrawListener;
import androidx.interpolator.view.animation.LinearOutSlowInInterpolator;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;

/**
 * Bottom navigation view with a gradient indicator below the selected item.
 */
public class BottomNavigationViewWithIndicator extends BottomNavigationView implements NavigationBarView.OnItemSelectedListener {

    private static final float DEFAULT_SCALE = 1f;
    private static final float MAX_SCALE = 2f;

    private static final long BASE_DURATION = 300L;
    private static final long VARIABLE_DURATION = 300L;

    private static final int SEPARATOR_HEIGHT_DP = 4;

    @Nullable
    private OnItemSelectedListener externalSelectedListener;
    @Nullable
    private ValueAnimator animator;

    private Drawable shape;

    private RectF indicator;
    private Rect bounds;

    private float defaultSizePx;
    private float separatorHeight;

    public BottomNavigationViewWithIndicator(@NonNull Context context) {
        super(context);
        init();
    }

    public BottomNavigationViewWithIndicator(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public BottomNavigationViewWithIndicator(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public BottomNavigationViewWithIndicator(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        super.setOnItemSelectedListener(this);

        shape = AppCompatResources.getDrawable(getContext(), R.drawable.bottom_navigation_gradient_separator);
        separatorHeight = ViewUtils.dpToPx(getContext(), SEPARATOR_HEIGHT_DP);
        indicator = new RectF();
        bounds = new Rect();
        defaultSizePx = 0.0f;
    }

    @Override
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {
        if (externalSelectedListener != null && externalSelectedListener.onNavigationItemSelected(item) && item.getItemId() != getSelectedItemId()) {
            onItemSelected(item.getItemId(), true);
            return true;
        }
        return false;
    }

    @Override
    public void setOnItemSelectedListener(@Nullable OnItemSelectedListener listener) {
        externalSelectedListener = listener;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        OneShotPreDrawListener.add(this, () -> {
            defaultSizePx = findViewById(getSelectedItemId()).getWidth();
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
        if (isLaidOut()) {
            indicator.roundOut(bounds);
            shape.setBounds(bounds);
            shape.draw(canvas);
        }
    }

    private void onItemSelected(int itemId, boolean animate) {
        if (!isLaidOut()) return;

        // Interrupt any current animation, but don't set the end values,
        // if it's in the middle of a movement we want it to start from
        // the current position, to make the transition smoother.
        cancelAnimator(false);

        final View itemView = findViewById(itemId);
        final float fromCenterX = indicator.centerX();
        final float fromScale = indicator.width() / defaultSizePx;

        animator = ValueAnimator.ofFloat(fromScale, MAX_SCALE, DEFAULT_SCALE);
        animator.addUpdateListener(animation -> {
            float progress = animation.getAnimatedFraction();
            float distanceTravelled = linearInterpolation(progress, fromCenterX, getCenterX(itemView));

            float scale = (float) animation.getAnimatedValue();
            float indicatorWidth = defaultSizePx * scale;

            float left = distanceTravelled - indicatorWidth / 2f;
            float top = getHeight() - separatorHeight;
            float right = distanceTravelled + indicatorWidth / 2f;
            float bottom = getHeight();

            indicator.set(left, top, right, bottom);
            invalidate();
        });

        animator.setInterpolator(new LinearOutSlowInInterpolator());


        if (animate) {
            float distanceToMove = Math.abs(fromCenterX - getCenterX(itemView));
            animator.setDuration(calculateDuration(distanceToMove));
        } else {
            animator.setDuration(0L);
        }

        animator.start();
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
        return (long) (BASE_DURATION + VARIABLE_DURATION * MathUtils.clamp(distance / getWidth(), 0f, 1f));
    }

    /**
     * Gets the center X value of a view.
     */
    private float getCenterX(@NonNull final View view) {
        return view.getLeft() + view.getWidth() / 2f;
    }

    private void cancelAnimator(final boolean setEndValues) {
        final ValueAnimator currentAnimator = animator;
        if (currentAnimator == null) return;

        if (setEndValues) {
            currentAnimator.end();
        } else {
            currentAnimator.cancel();
        }
        currentAnimator.removeAllUpdateListeners();
        animator = null;
    }
}

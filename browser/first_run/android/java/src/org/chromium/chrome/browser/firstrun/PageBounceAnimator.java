/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.firstrun;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.TimeInterpolator;
import android.animation.ValueAnimator;
import android.view.View;
import android.view.animation.PathInterpolator;

import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Drives programmatic {@link ViewPager2} transitions with a smooth, elastic feel by animating its
 * internal {@link RecyclerView} directly. This keeps the scroll math deterministic and lets us add
 * a lightweight visual bounce without overshooting the target page. {@link ViewPager2} is built on
 * top of {@link RecyclerView}, so using its internal child here is safe and consistent with the
 * platform implementation.
 */
@NullMarked
public class PageBounceAnimator {
    /** Duration of the page transition animation. */
    private static final int ANIMATION_DURATION_MS = 520;

    /** Visual bounce distance in dp applied after alignment. */
    private static final float BOUNCE_DISTANCE_DP = 8f;

    /** Duration for the visual bounce. */
    private static final int BOUNCE_DURATION_MS = 180;

    /** Interpolator for a smooth, non-overshooting curve. */
    private static final TimeInterpolator PATH_INTERPOLATOR =
            new PathInterpolator(0.6f, 0f, 0.2f, 1f);

    /** ViewPager2 hosting the pages. */
    private final ViewPager2 mViewPager;

    /** RecyclerView inside ViewPager2 that actually scrolls. */
    private final RecyclerView mRecyclerView;

    private final float mAmplitude;

    @Nullable private ValueAnimator mPageAnimator;
    @Nullable private ValueAnimator mBounceAnimator;

    /** Listener called when a page finishes settling. */
    public interface OnPageShownListener {
        void onPageShown(final int position);
    }

    /** Creates a new animator bound to the provided ViewPager2. */
    public PageBounceAnimator(final ViewPager2 viewPager) {
        mViewPager = viewPager;
        final View child = viewPager.getChildAt(0);
        mRecyclerView = (RecyclerView) child;
        final float density = mViewPager.getResources().getDisplayMetrics().density;
        mAmplitude = BOUNCE_DISTANCE_DP * density;
        mViewPager.addOnAttachStateChangeListener(
                new View.OnAttachStateChangeListener() {
                    @Override
                    public void onViewAttachedToWindow(View view) {
                        /* No-op. */
                    }

                    @Override
                    public void onViewDetachedFromWindow(View view) {
                        // Stop any running animation to avoid leaks or callbacks into a detached
                        // view.
                        if (mPageAnimator != null && mPageAnimator.isRunning()) {
                            mPageAnimator.cancel();
                        }
                        if (mBounceAnimator != null && mBounceAnimator.isRunning()) {
                            mBounceAnimator.cancel();
                        }
                    }
                });
    }

    /** Animates to the target adapter position with a bouncy overshoot. */
    public void animateToPosition(final int targetPosition) {
        if (mPageAnimator != null && mPageAnimator.isRunning()) {
            return;
        }
        if (mBounceAnimator != null && mBounceAnimator.isRunning()) {
            return;
        }

        final int current = mViewPager.getCurrentItem();
        if (targetPosition == current) {
            return;
        }

        final int pageWidth =
                mViewPager.getWidth() - mViewPager.getPaddingLeft() - mViewPager.getPaddingRight();
        final int startX = mRecyclerView.computeHorizontalScrollOffset();
        final int targetX = targetPosition * pageWidth;
        if (pageWidth == 0) {
            mViewPager.setCurrentItem(targetPosition, false);
            return;
        }

        mPageAnimator = ValueAnimator.ofFloat(0f, 1f);
        mPageAnimator.setDuration(ANIMATION_DURATION_MS);
        mPageAnimator.setInterpolator(PATH_INTERPOLATOR);

        mPageAnimator.addUpdateListener(
                animation -> {
                    final float t = (float) animation.getAnimatedValue();
                    final float desired = startX + (targetX - startX) * t;
                    final int desiredInt = Math.round(desired);
                    final int currentX = mRecyclerView.computeHorizontalScrollOffset();
                    final int delta = desiredInt - currentX;
                    if (delta != 0) {
                        mRecyclerView.scrollBy(delta, 0);
                    }
                });

        mPageAnimator.addListener(
                new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        playBounce(targetPosition);
                    }

                    @Override
                    public void onAnimationCancel(Animator animation) {
                        playBounce(targetPosition);
                    }
                });

        mPageAnimator.start();
    }

    /** Returns the adapter position of the currently snapped page. */
    public int getCurrentPosition() {
        return mViewPager.getCurrentItem();
    }

    private void playBounce(final int targetPosition) {
        if (mBounceAnimator != null && mBounceAnimator.isRunning()) {
            mBounceAnimator.cancel();
        }
        mBounceAnimator = ValueAnimator.ofFloat(0f, mAmplitude, 0f);
        mBounceAnimator.setDuration(BOUNCE_DURATION_MS);
        mBounceAnimator.addUpdateListener(
                animation -> {
                    float value = (float) animation.getAnimatedValue();
                    mRecyclerView.setTranslationX(value);
                });
        mBounceAnimator.addListener(
                new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        finalizeTransition(targetPosition, false);
                    }

                    @Override
                    public void onAnimationCancel(Animator animation) {
                        finalizeTransition(targetPosition, true);
                    }
                });
        mBounceAnimator.start();
    }

    private void finalizeTransition(int targetPosition, boolean smoothScroll) {
        mRecyclerView.setTranslationX(0f);
        mViewPager.setCurrentItem(targetPosition, smoothScroll);
    }
}

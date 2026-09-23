/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;

import androidx.annotation.VisibleForTesting;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Observes the New Tab Page, measures the region that is free of native browser UI and reports it
 * in CSS pixels to the sponsored rich media WebView.
 */
@NullMarked
public class NewTabTakeoverSafeAreaReporter extends RecyclerView.OnScrollListener
        implements ViewTreeObserver.OnGlobalLayoutListener {
    // Time interval to debounce multiple measurements. Only one measurement is posted within this
    // interval.
    private static final long MEASUREMENT_DELAY_MS = 120;

    // Matches the margin used on the New Tab Page.
    private static final float SAFE_AREA_MARGIN_DP = BraveNtpAdapter.CARD_MARGIN_DP;

    private final BraveNewTabPageLayout mNtpLayout;
    private final RecyclerView mRecyclerView;
    private final BraveNtpAdapter mNtpAdapter;
    private final FrameLayout mRichMediaContainer;
    private final SponsoredRichMediaWebView mRichMediaWebView;

    private @Nullable Handler mMeasurementHandler = new Handler(Looper.getMainLooper());
    private @Nullable Integer mCachedTopNtpItemsHeight;
    private @Nullable Integer mCachedTopNtpItemsCount;

    public NewTabTakeoverSafeAreaReporter(
            BraveNewTabPageLayout ntpLayout,
            RecyclerView recyclerView,
            BraveNtpAdapter ntpAdapter,
            FrameLayout richMediaContainer,
            SponsoredRichMediaWebView richMediaWebView) {
        mNtpLayout = ntpLayout;
        mRecyclerView = recyclerView;
        mNtpAdapter = ntpAdapter;
        mRichMediaContainer = richMediaContainer;
        mRichMediaWebView = richMediaWebView;

        mNtpLayout.getViewTreeObserver().addOnGlobalLayoutListener(this);
        mRecyclerView.addOnScrollListener(this);

        scheduleMeasurement();
    }

    public void destroy() {
        if (mMeasurementHandler != null) {
            mMeasurementHandler.removeCallbacksAndMessages(null);
            mMeasurementHandler = null;
        }

        mNtpLayout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
        mRecyclerView.removeOnScrollListener(this);
    }

    public void scheduleMeasurement() {
        if (mMeasurementHandler == null) {
            return;
        }

        mMeasurementHandler.removeCallbacksAndMessages(null);
        mMeasurementHandler.postDelayed(this::measureAndReportSafeArea, MEASUREMENT_DELAY_MS);
    }

    @Override
    public void onGlobalLayout() {
        scheduleMeasurement();
    }

    @Override
    public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
        if (newState == RecyclerView.SCROLL_STATE_IDLE) {
            scheduleMeasurement();
        }
    }

    private void measureAndReportSafeArea() {
        final int viewportWidth = mRichMediaContainer.getWidth();
        final int viewportHeight = mRichMediaContainer.getHeight();
        if (viewportWidth == 0 || viewportHeight == 0) {
            return;
        }

        @Nullable Integer topNtpItemsHeight = getTopNtpItemsHeight();
        if (topNtpItemsHeight == null) {
            return;
        }

        final int safeAreaTop =
                mRecyclerView.getTop() - mRichMediaContainer.getTop() + topNtpItemsHeight;

        mRichMediaWebView.setSafeArea(
                calculateSafeArea(
                        safeAreaTop,
                        safeAreaTop + mNtpAdapter.getTopMarginImageCredit(),
                        viewportWidth,
                        viewportHeight,
                        mNtpLayout.getResources().getDisplayMetrics().density));
    }

    private @Nullable Integer getTopNtpItemsHeight() {
        if (!(mRecyclerView.getLayoutManager() instanceof LinearLayoutManager layoutManager)) {
            return null;
        }

        @Nullable Integer topNtpItemsHeight = calculateTopNtpItemsHeightFromLayout(layoutManager);
        if (topNtpItemsHeight == null) {
            if (mCachedTopNtpItemsHeight == null || mCachedTopNtpItemsCount == null) {
                return null;
            }
            // Reuse the cached height only if the top items count hasn't changed.
            if (mNtpAdapter.getTopItemsCount() != mCachedTopNtpItemsCount) {
                return null;
            }
            topNtpItemsHeight = mCachedTopNtpItemsHeight;
        }

        mCachedTopNtpItemsHeight = topNtpItemsHeight;
        mCachedTopNtpItemsCount = mNtpAdapter.getTopItemsCount();

        return topNtpItemsHeight;
    }

    private @Nullable Integer calculateTopNtpItemsHeightFromLayout(
            LinearLayoutManager layoutManager) {
        int topNtpItemsHeight = 0;
        for (int position = 0; position < mNtpAdapter.getTopItemsCount(); position++) {
            View contentView = layoutManager.findViewByPosition(position);
            if (contentView == null) {
                return null;
            }
            topNtpItemsHeight += getHeightWithMargins(contentView);
        }
        return topNtpItemsHeight;
    }

    private static int getHeightWithMargins(View view) {
        int height = view.getHeight();
        if (view.getLayoutParams() instanceof ViewGroup.MarginLayoutParams layoutParams) {
            height += layoutParams.topMargin + layoutParams.bottomMargin;
        }
        return height;
    }

    /**
     * Returns the region between `safeAreaTop` and `safeAreaBottom` in dp units (which match CSS
     * pixels in a WebView without a custom viewport scale), inset by the NTP content margin and
     * clamped to the viewport.
     */
    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static RectF calculateSafeArea(
            int safeAreaTop,
            int safeAreaBottom,
            int viewportWidth,
            int viewportHeight,
            float density) {
        final float viewportWidthDp = viewportWidth / density;
        final float viewportHeightDp = viewportHeight / density;

        final float bottom =
                clamp(safeAreaBottom / density - SAFE_AREA_MARGIN_DP, 0, viewportHeightDp);
        final float top = clamp(safeAreaTop / density + SAFE_AREA_MARGIN_DP, 0, bottom);

        final float right = clamp(viewportWidthDp - SAFE_AREA_MARGIN_DP, 0, viewportWidthDp);
        final float left = clamp(SAFE_AREA_MARGIN_DP, 0, right);

        return new RectF(left, top, right, bottom);
    }

    private static float clamp(float value, float min, float max) {
        return Math.max(min, Math.min(value, max));
    }
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.res.Resources;
import android.graphics.RectF;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.BaseRobolectricTestRunner;

import java.util.concurrent.TimeUnit;

@RunWith(BaseRobolectricTestRunner.class)
@Config(
        manifest = Config.NONE,
        shadows = {ShadowLooper.class})
public class NewTabTakeoverSafeAreaReporterTest {
    // Local copy of the private production constant so the test doesn't need
    // package-visibility changes just to reference MEASUREMENT_DELAY_MS.
    private static final long MEASUREMENT_DELAY_MS = 120;

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveNewTabPageLayout mNtpLayout;
    @Mock private RecyclerView mRecyclerView;
    @Mock private BraveNtpAdapter mNtpAdapter;
    @Mock private FrameLayout mRichMediaContainer;
    @Mock private SponsoredRichMediaWebView mRichMediaWebView;
    @Mock private ViewTreeObserver mViewTreeObserver;
    @Mock private LinearLayoutManager mLayoutManager;

    @Before
    public void setUp() {
        when(mNtpLayout.getViewTreeObserver()).thenReturn(mViewTreeObserver);
        // Mocked Resources with a pinned, non-default density so the test can tell "read
        // DisplayMetrics.density" apart from "hardcode 1f" (Robolectric's implicit default
        // density is typically 1f).
        Resources resources = mock(Resources.class);
        DisplayMetrics metrics = new DisplayMetrics();
        metrics.density = 2f;
        when(resources.getDisplayMetrics()).thenReturn(metrics);
        when(mNtpLayout.getResources()).thenReturn(resources);
    }

    private NewTabTakeoverSafeAreaReporter createReporter() {
        return new NewTabTakeoverSafeAreaReporter(
                mNtpLayout, mRecyclerView, mNtpAdapter, mRichMediaContainer, mRichMediaWebView);
    }

    /**
     * Stubs a full, internally-consistent geometry: nonzero unequal viewport dimensions, two
     * distinct top items with different heights and unequal margins, and distinct nonzero
     * container/recycler top offsets. Distinctive values throughout so a swapped operand, an
     * ignored margin, or a dropped item would change the reported RectF.
     */
    private void stubConsistentGeometry() {
        when(mRichMediaContainer.getWidth()).thenReturn(1080);
        when(mRichMediaContainer.getHeight()).thenReturn(1920);
        when(mRichMediaContainer.getTop()).thenReturn(50);
        when(mRecyclerView.getTop()).thenReturn(300);
        when(mRecyclerView.getLayoutManager()).thenReturn(mLayoutManager);
        when(mNtpAdapter.getTopItemsCount()).thenReturn(2);
        when(mNtpAdapter.getTopMarginImageCredit()).thenReturn(90);

        when(mLayoutManager.findViewByPosition(0)).thenReturn(stubTopItemView(100, 10, 5));
        when(mLayoutManager.findViewByPosition(1)).thenReturn(stubTopItemView(50, 4, 8));
    }

    private static View stubTopItemView(int height, int topMargin, int bottomMargin) {
        View view = mock(View.class);
        when(view.getHeight()).thenReturn(height);
        ViewGroup.MarginLayoutParams layoutParams = new ViewGroup.MarginLayoutParams(0, 0);
        layoutParams.topMargin = topMargin;
        layoutParams.bottomMargin = bottomMargin;
        when(view.getLayoutParams()).thenReturn(layoutParams);
        return view;
    }

    private static RectF expectedSafeAreaForConsistentGeometry() {
        int topItemsHeight = (100 + 10 + 5) + (50 + 4 + 8); // 177
        int safeAreaTop = 300 - 50 + topItemsHeight; // recycler.getTop() - container.getTop()
        int safeAreaBottom = safeAreaTop + 90; // + getTopMarginImageCredit()
        return NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                safeAreaTop,
                safeAreaBottom,
                /* viewportWidth= */ 1080,
                /* viewportHeight= */ 1920,
                /* density= */ 2f);
    }

    @Test
    public void spansTheGapBetweenTheTopBorderAndTheBottomBorder() {
        assertEquals(
                new RectF(16, 616, 1064, 1484),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void isEmptyWhenTheTopBorderAndBottomBorderAreEqual() {
        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 800,
                        /* safeAreaBottom= */ 800,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f);

        assertEquals(0f, safeArea.height(), 0f);
    }

    @Test
    public void clampsTheTopBorderToTheBottomBorderWhenTopExceedsBottom() {
        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 1600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f);

        assertEquals(new RectF(16, 1484, 1064, 1484), safeArea);
    }

    @Test
    public void clampsBothBordersToTheViewportBottomWhenBothExceedIt() {
        assertEquals(
                new RectF(16, 1920, 1064, 1920),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 2400,
                        /* safeAreaBottom= */ 2400,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsTheBottomBorderToTheViewportBottom() {
        assertEquals(
                new RectF(16, 616, 1064, 1920),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 2400,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsTheTopBorderToTheViewportTop() {
        assertEquals(
                new RectF(16, 0, 1064, 1484),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ -200,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f));
    }

    @Test
    public void clampsBothBordersToTheViewportTopWhenNegative() {
        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ -500,
                        /* safeAreaBottom= */ -200,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 1f);

        assertEquals(new RectF(16, 0, 1064, 0), safeArea);
    }

    @Test
    public void convertsViewPixelsToCssPixels() {
        assertEquals(
                new RectF(16, 316, 524, 734),
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 2f));

        RectF safeArea =
                NewTabTakeoverSafeAreaReporter.calculateSafeArea(
                        /* safeAreaTop= */ 600,
                        /* safeAreaBottom= */ 1500,
                        /* viewportWidth= */ 1080,
                        /* viewportHeight= */ 1920,
                        /* density= */ 2.625f);

        assertEquals(16f, safeArea.left, 0f);
        assertEquals(244.571f, safeArea.top, 0.001f);
        assertEquals(395.429f, safeArea.right, 0.001f);
        assertEquals(555.429f, safeArea.bottom, 0.001f);
    }

    @Test
    public void constructorRegistersListenersAndReportsSafeAreaAfterTheDelay() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();

        verify(mViewTreeObserver).addOnGlobalLayoutListener(reporter);
        verify(mRecyclerView).addOnScrollListener(reporter);

        // The report is genuinely delayed: an eager no-delay post() must not pass this.
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS - 1, TimeUnit.MILLISECONDS);
        verify(mRichMediaWebView, never()).setSafeArea(any());

        ShadowLooper.idleMainLooper(1, TimeUnit.MILLISECONDS);
        ArgumentCaptor<RectF> safeAreaCaptor = ArgumentCaptor.forClass(RectF.class);
        verify(mRichMediaWebView).setSafeArea(safeAreaCaptor.capture());
        assertEquals(expectedSafeAreaForConsistentGeometry(), safeAreaCaptor.getValue());
    }

    @Test
    public void onGlobalLayoutSchedulesAMeasurement() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        clearInvocations(mRichMediaWebView);

        reporter.onGlobalLayout();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, times(1)).setSafeArea(any());
    }

    @Test
    public void onScrollStateIdleSchedulesAMeasurement() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        clearInvocations(mRichMediaWebView);

        reporter.onScrollStateChanged(mRecyclerView, RecyclerView.SCROLL_STATE_IDLE);
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, times(1)).setSafeArea(any());
    }

    @Test
    public void onScrollStateDraggingDoesNotScheduleAMeasurement() {
        stubConsistentGeometry();
        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        clearInvocations(mRichMediaWebView);

        reporter.onScrollStateChanged(mRecyclerView, RecyclerView.SCROLL_STATE_DRAGGING);
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void onScrollStateSettlingDoesNotScheduleAMeasurement() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        clearInvocations(mRichMediaWebView);

        reporter.onScrollStateChanged(mRecyclerView, RecyclerView.SCROLL_STATE_SETTLING);
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void debouncesMultipleScheduleCallsIntoOneMeasurement() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        clearInvocations(mRichMediaWebView);

        reporter.scheduleMeasurement();
        reporter.scheduleMeasurement();
        reporter.scheduleMeasurement();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, times(1)).setSafeArea(any());
    }

    @Test
    public void destroyCancelsThePendingMeasurement() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();

        // Do not idle first: the still-pending constructor callback is what destroy() must
        // cancel.
        reporter.destroy();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
        verify(mViewTreeObserver).removeOnGlobalLayoutListener(reporter);
        verify(mRecyclerView).removeOnScrollListener(reporter);
    }

    @Test
    public void scheduleMeasurementIsANoOpAfterDestroy() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        clearInvocations(mRichMediaWebView);
        reporter.destroy();

        reporter.scheduleMeasurement();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void skipsMeasurementWhenViewportWidthIsZero() {
        when(mRecyclerView.getLayoutManager()).thenReturn(mLayoutManager);
        when(mNtpAdapter.getTopItemsCount()).thenReturn(0);
        when(mRichMediaContainer.getWidth()).thenReturn(0);
        when(mRichMediaContainer.getHeight()).thenReturn(1920);

        createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void skipsMeasurementWhenViewportHeightIsZero() {
        when(mRecyclerView.getLayoutManager()).thenReturn(mLayoutManager);
        when(mNtpAdapter.getTopItemsCount()).thenReturn(0);
        when(mRichMediaContainer.getWidth()).thenReturn(1080);
        when(mRichMediaContainer.getHeight()).thenReturn(0);

        createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void skipsMeasurementWhenLayoutManagerIsNotLinear() {
        when(mRichMediaContainer.getWidth()).thenReturn(1080);
        when(mRichMediaContainer.getHeight()).thenReturn(1920);
        when(mNtpAdapter.getTopItemsCount()).thenReturn(0);
        // A plain (non-Linear) LayoutManager, not null: findViewByPosition is declared on the
        // base LayoutManager class, so if the instanceof check were deleted, a count of 0 would
        // still assign mTopNtpItemsHeight = 0 and incorrectly report. A null layout manager
        // would still no-op via a trivial null check and wouldn't catch that regression.
        // Must not be a GridLayoutManager: it extends LinearLayoutManager, so instanceof would
        // still succeed.
        when(mRecyclerView.getLayoutManager()).thenReturn(mock(RecyclerView.LayoutManager.class));

        createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void skipsMeasurementWhenATopItemViewIsNotYetLaidOut() {
        when(mRichMediaContainer.getWidth()).thenReturn(1080);
        when(mRichMediaContainer.getHeight()).thenReturn(1920);
        when(mRecyclerView.getLayoutManager()).thenReturn(mLayoutManager);
        when(mNtpAdapter.getTopItemsCount()).thenReturn(2);
        when(mLayoutManager.findViewByPosition(0)).thenReturn(stubTopItemView(100, 0, 0));
        when(mLayoutManager.findViewByPosition(1)).thenReturn(null);

        createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        verify(mRichMediaWebView, never()).setSafeArea(any());
    }

    @Test
    public void usesCachedTopItemsHeightWhenATopItemViewIsLaterMissing() {
        stubConsistentGeometry();

        NewTabTakeoverSafeAreaReporter reporter = createReporter();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);
        ArgumentCaptor<RectF> firstCaptor = ArgumentCaptor.forClass(RectF.class);
        verify(mRichMediaWebView).setSafeArea(firstCaptor.capture());
        clearInvocations(mRichMediaWebView);

        // The top item view is no longer laid out (e.g. scrolled off), but the cached height
        // from the first successful measurement must still be used.
        when(mLayoutManager.findViewByPosition(0)).thenReturn(null);
        reporter.scheduleMeasurement();
        ShadowLooper.idleMainLooper(MEASUREMENT_DELAY_MS, TimeUnit.MILLISECONDS);

        ArgumentCaptor<RectF> secondCaptor = ArgumentCaptor.forClass(RectF.class);
        verify(mRichMediaWebView).setSafeArea(secondCaptor.capture());
        assertEquals(firstCaptor.getValue(), secondCaptor.getValue());
    }
}

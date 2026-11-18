/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Activity;
import android.content.Context;
import android.os.Looper;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.prefs.PrefService;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link NewTabTakeoverDisclosureTooltipController}. */
@RunWith(BaseRobolectricTestRunner.class)
public class NewTabTakeoverDisclosureTooltipControllerTest {
    private static final int REMAINING_DISPLAY_COUNT = 5;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private PrefService mPrefService;

    private int mRemainingDisplayCount = REMAINING_DISPLAY_COUNT;
    private boolean mIsRewardsEnabled;

    private final List<NewTabTakeoverDisclosureTooltip> mTooltips = new ArrayList<>();
    private NewTabTakeoverDisclosureTooltip.Delegate mTooltipDelegate;

    private FrameLayout mRootView;
    private TextView mDisclosureLabel;
    private NewTabTakeoverDisclosureTooltipController mController;

    @Before
    public void setUp() {
        when(mPrefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT))
                .thenAnswer(invocation -> mRemainingDisplayCount);
        doAnswer(
                        invocation -> {
                            mRemainingDisplayCount = invocation.getArgument(1);
                            return null;
                        })
                .when(mPrefService)
                .setInteger(
                        eq(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT), anyInt());

        Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
        mRootView = new FrameLayout(activity);
        mDisclosureLabel = new TextView(activity);
        mDisclosureLabel.setId(R.id.sponsored_label);
        mRootView.addView(mDisclosureLabel, new FrameLayout.LayoutParams(100, 40));
        activity.setContentView(mRootView);
        runUiTasks();
    }

    private NewTabTakeoverDisclosureTooltipController createController() {
        return new NewTabTakeoverDisclosureTooltipController(
                mRootView, mPrefService, () -> mIsRewardsEnabled, this::createTooltip);
    }

    private NewTabTakeoverDisclosureTooltip createTooltip(
            Context context,
            View disclosureLabel,
            NewTabTakeoverDisclosureTooltip.Delegate delegate) {
        NewTabTakeoverDisclosureTooltip tooltip = mock(NewTabTakeoverDisclosureTooltip.class);
        mTooltipDelegate = delegate;
        mTooltips.add(tooltip);
        return tooltip;
    }

    private void runUiTasks() {
        shadowOf(Looper.getMainLooper()).runToEndOfTasks();
    }

    private void layout() {
        layoutWithoutRunningUiTasks();
        runUiTasks();
    }

    private void layoutWithoutRunningUiTasks() {
        mRootView.getViewTreeObserver().dispatchOnGlobalLayout();
    }

    private void advanceTimeBy(long milliseconds) {
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(milliseconds));
    }

    private NewTabTakeoverDisclosureTooltip getLastTooltip() {
        return mTooltips.get(mTooltips.size() - 1);
    }

    private NewTabTakeoverDisclosureTooltip autoDisplay() {
        mController = createController();
        layout();
        return getLastTooltip();
    }

    @Test
    public void testAutoDisplayWhenDisplaysRemain() {
        NewTabTakeoverDisclosureTooltip tooltip = autoDisplay();

        assertEquals(1, mTooltips.size());
        verify(tooltip).show();
    }

    @Test
    public void testDoNotAutoDisplayBeforeDelayElapses() {
        mController = createController();
        layoutWithoutRunningUiTasks();

        advanceTimeBy(NewTabTakeoverDisclosureTooltipController.AUTO_DISPLAY_DELAY_MS - 1);

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testAutoDisplayWhenDelayElapses() {
        mController = createController();
        layoutWithoutRunningUiTasks();

        advanceTimeBy(NewTabTakeoverDisclosureTooltipController.AUTO_DISPLAY_DELAY_MS);

        assertEquals(1, mTooltips.size());
    }

    @Test
    public void testKeepClickedTooltipWhenDelayElapsesAfterLabelIsClicked() {
        mController = createController();
        layoutWithoutRunningUiTasks();
        mController.onDisclosureLabelClicked(mDisclosureLabel);
        NewTabTakeoverDisclosureTooltip clickedTooltip = getLastTooltip();

        runUiTasks();

        verify(clickedTooltip, never()).dismiss();
        assertEquals(1, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT - 1, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayWhenDismissedBeforeDelayElapses() {
        mController = createController();
        layoutWithoutRunningUiTasks();

        mController.dismiss();
        runUiTasks();

        assertEquals(0, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayWhenDisclosureLabelIsHiddenBeforeDelayElapses() {
        mController = createController();
        layoutWithoutRunningUiTasks();

        mDisclosureLabel.setVisibility(View.GONE);
        runUiTasks();

        assertEquals(0, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayWhenNoDisplaysRemain() {
        mRemainingDisplayCount = 0;

        mController = createController();
        layout();

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testDoNotAutoDisplayWhenRemainingDisplayCountIsNegative() {
        mRemainingDisplayCount = -1;

        mController = createController();
        layout();

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testDoNotAutoDisplayWhenRewardsIsEnabled() {
        mIsRewardsEnabled = true;

        mController = createController();
        layout();

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testDoNotAutoDisplayWhenDisclosureLabelIsHidden() {
        mDisclosureLabel.setVisibility(View.GONE);

        mController = createController();
        layout();

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testAutoDisplayWhenDisclosureLabelIsShown() {
        mDisclosureLabel.setVisibility(View.GONE);
        mController = createController();
        layout();

        mDisclosureLabel.setVisibility(View.VISIBLE);
        layout();

        assertEquals(1, mTooltips.size());
    }

    @Test
    public void testDoNotAutoDisplayWhenDisclosureLabelIsTransparent() {
        mDisclosureLabel.setAlpha(0f);

        mController = createController();
        layout();

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testDoNotAutoDisplayWhenRewardsIsEnabledBeforeDisclosureLabelIsShown() {
        mDisclosureLabel.setVisibility(View.GONE);
        mController = createController();
        layout();

        mIsRewardsEnabled = true;
        mDisclosureLabel.setVisibility(View.VISIBLE);
        layout();

        assertEquals(0, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayWhenNoDisplaysRemainBeforeDisclosureLabelIsShown() {
        mDisclosureLabel.setVisibility(View.GONE);
        mController = createController();
        layout();

        mRemainingDisplayCount = 0;
        mDisclosureLabel.setVisibility(View.VISIBLE);
        layout();

        assertEquals(0, mTooltips.size());
        assertEquals(0, mRemainingDisplayCount);
    }

    @Test
    public void testDecrementRemainingDisplayCountWhenAutoDisplayed() {
        autoDisplay();

        assertEquals(REMAINING_DISPLAY_COUNT - 1, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayAgainWhenLayoutRunsAfterTooltipIsDismissed() {
        autoDisplay();
        mController.dismiss();

        layout();

        assertEquals(1, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT - 1, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayOnceRemainingDisplayCountIsExhausted() {
        mRemainingDisplayCount = 1;
        autoDisplay();
        mController.dismiss();

        mController = createController();
        layout();

        assertEquals(1, mTooltips.size());
        assertEquals(0, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotDecrementRemainingDisplayCountWhenLabelIsClickedAndRewardsIsEnabled() {
        mDisclosureLabel.setVisibility(View.GONE);
        mController = createController();
        mIsRewardsEnabled = true;

        mController.onDisclosureLabelClicked(mDisclosureLabel);

        assertEquals(1, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT, mRemainingDisplayCount);
    }

    @Test
    public void testSuppressAutoDisplayWhenLearnMoreIsClicked() {
        NewTabTakeoverDisclosureTooltip tooltip = autoDisplay();

        mTooltipDelegate.onLearnMoreClicked(tooltip);

        verify(tooltip).dismiss();
        assertEquals(0, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotAutoDisplayAfterLearnMoreIsClicked() {
        mController = createController();
        mController.onDisclosureLabelClicked(mDisclosureLabel);
        mTooltipDelegate.onLearnMoreClicked(getLastTooltip());

        mController = createController();
        layout();

        assertEquals(1, mTooltips.size());
    }

    @Test
    public void testDismissShownTooltipWhenControllerIsDismissed() {
        NewTabTakeoverDisclosureTooltip tooltip = autoDisplay();

        mController.dismiss();

        verify(tooltip).dismiss();
    }

    @Test
    public void testDoNotDismissTooltipWhenPreviousPopupWindowTooltipIsDismissed() {
        mController = createController();
        mController.onDisclosureLabelClicked(mDisclosureLabel);
        NewTabTakeoverDisclosureTooltip previousTooltip = getLastTooltip();
        mController.dismiss();
        mController.onDisclosureLabelClicked(mDisclosureLabel);
        NewTabTakeoverDisclosureTooltip tooltip = getLastTooltip();

        mTooltipDelegate.onPopupWindowTooltipDismissed(previousTooltip);
        mController.onDisclosureLabelClicked(mDisclosureLabel);

        verify(tooltip, never()).dismiss();
        assertEquals(2, mTooltips.size());
    }

    @Test
    public void testDoNotAutoDisplayAfterControllerIsDismissed() {
        mDisclosureLabel.setVisibility(View.GONE);
        mController = createController();

        mController.dismiss();
        mDisclosureLabel.setVisibility(View.VISIBLE);
        layout();

        assertEquals(0, mTooltips.size());
    }

    @Test
    public void testDoNotDecrementRemainingDisplayCountWhenLabelIsClickedAfterAutoDisplay() {
        autoDisplay();
        mController.dismiss();

        mController.onDisclosureLabelClicked(mDisclosureLabel);

        verify(getLastTooltip()).show();
        assertEquals(2, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT - 1, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotDecrementRemainingDisplayCountWhenLabelIsClickedAndNoDisplaysRemain() {
        mRemainingDisplayCount = 0;
        mController = createController();

        mController.onDisclosureLabelClicked(mDisclosureLabel);

        verify(getLastTooltip()).show();
        assertEquals(0, mRemainingDisplayCount);
    }

    @Test
    public void testKeepClickedTooltipWhenDisclosureLabelIsShownAfterLabelIsClicked() {
        mDisclosureLabel.setVisibility(View.GONE);
        mController = createController();
        mController.onDisclosureLabelClicked(mDisclosureLabel);
        NewTabTakeoverDisclosureTooltip clickedTooltip = getLastTooltip();

        mDisclosureLabel.setVisibility(View.VISIBLE);
        layout();

        verify(clickedTooltip, never()).dismiss();
        assertEquals(1, mTooltips.size());
        assertEquals(REMAINING_DISPLAY_COUNT - 1, mRemainingDisplayCount);
    }

    @Test
    public void testDoNotOpenAnotherTooltipWhenLabelIsClickedWhileOneIsShown() {
        mController = createController();
        mController.onDisclosureLabelClicked(mDisclosureLabel);

        mController.onDisclosureLabelClicked(mDisclosureLabel);

        assertEquals(1, mTooltips.size());
    }

    @Test
    public void testOpenNewTooltipWhenLabelIsClickedAfterPopupWindowTooltipIsDismissed() {
        mController = createController();
        mController.onDisclosureLabelClicked(mDisclosureLabel);
        mTooltipDelegate.onPopupWindowTooltipDismissed(getLastTooltip());

        mController.onDisclosureLabelClicked(mDisclosureLabel);

        assertEquals(2, mTooltips.size());
    }
}

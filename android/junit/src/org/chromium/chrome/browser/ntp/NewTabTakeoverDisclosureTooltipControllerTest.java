/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.prefs.PrefService;

/** Unit tests for {@link NewTabTakeoverDisclosureTooltipController}. */
@RunWith(BaseRobolectricTestRunner.class)
public class NewTabTakeoverDisclosureTooltipControllerTest {
    private static final int REMAINING_DISPLAY_COUNT = 5;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private PrefService mPrefService;

    private int mRemainingDisplayCount = REMAINING_DISPLAY_COUNT;
    private boolean mIsRewardsEnabled;
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

        mController =
                new NewTabTakeoverDisclosureTooltipController(
                        mPrefService, () -> mIsRewardsEnabled);
    }

    private void setRemainingDisplayCount(int count) {
        mRemainingDisplayCount = count;
    }

    private int getRemainingDisplayCount() {
        return mRemainingDisplayCount;
    }

    @Test
    public void testAutoDisplaysWhenDisplaysRemain() {
        assertTrue(mController.shouldAutoDisplay());
    }

    @Test
    public void testDoesNotAutoDisplayWhenNoDisplaysRemain() {
        setRemainingDisplayCount(0);

        assertFalse(mController.shouldAutoDisplay());
    }

    @Test
    public void testDoesNotAutoDisplayWhenRemainingDisplayCountIsNegative() {
        setRemainingDisplayCount(-1);

        assertFalse(mController.shouldAutoDisplay());
    }

    @Test
    public void testDoesNotAutoDisplayWhenRewardsIsEnabled() {
        mIsRewardsEnabled = true;

        assertFalse(mController.shouldAutoDisplay());
    }

    @Test
    public void testRecordingAnAutoDisplayDecrementsTheRemainingDisplayCount() {
        mController.onAutoDisplayed();

        assertEquals(REMAINING_DISPLAY_COUNT - 1, getRemainingDisplayCount());
    }

    @Test
    public void testDoesNotAutoDisplayOnceTheRemainingDisplayCountIsExhausted() {
        for (int i = 0; i < REMAINING_DISPLAY_COUNT; ++i) {
            new NewTabTakeoverDisclosureTooltipController(mPrefService, () -> mIsRewardsEnabled)
                    .onAutoDisplayed();
        }

        assertFalse(
                new NewTabTakeoverDisclosureTooltipController(mPrefService, () -> mIsRewardsEnabled)
                        .shouldAutoDisplay());
        assertEquals(0, getRemainingDisplayCount());
    }

    @Test
    public void testDoesNotAutoDisplayAgainOnceAutoDisplayed() {
        mController.onAutoDisplayed();

        assertFalse(mController.shouldAutoDisplay());
    }

    @Test
    public void testDoesNotDecrementTheRemainingDisplayCountWhenNoDisplaysRemain() {
        setRemainingDisplayCount(0);

        mController.onAutoDisplayed();

        assertEquals(0, getRemainingDisplayCount());
        assertFalse(mController.shouldAutoDisplay());
    }

    @Test
    public void testDoesNotDecrementTheRemainingDisplayCountWhenRewardsIsEnabled() {
        mIsRewardsEnabled = true;

        mController.onAutoDisplayed();

        assertEquals(REMAINING_DISPLAY_COUNT, getRemainingDisplayCount());
    }

    @Test
    public void testClickingLearnMoreStopsFurtherAutoDisplays() {
        mController.onLearnMoreClicked();

        assertEquals(0, getRemainingDisplayCount());
        assertFalse(mController.shouldAutoDisplay());
    }

    @Test
    public void testCheckingWhetherToAutoDisplayDoesNotChangeTheRemainingDisplayCount() {
        mController.shouldAutoDisplay();

        assertEquals(REMAINING_DISPLAY_COUNT, getRemainingDisplayCount());
    }
}

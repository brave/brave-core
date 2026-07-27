/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.snackbar;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.widget.FrameLayout;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;

/**
 * Unit tests for the New Tab Takeover dedup bookkeeping in {@link BraveSnackbarManager}. Each NTP
 * tab creates its own notice but they share the window's manager, so the manager must report a
 * single outstanding notice and release it once dismissed.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@CommandLineFlags.Add({
    ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
    ChromeSwitches.DISABLE_NATIVE_INITIALIZATION
})
public class BraveSnackbarManagerNewTabTakeoverTest {
    @Rule
    public ActivityScenarioRule<ChromeTabbedActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(ChromeTabbedActivity.class);

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    private BraveSnackbarManager mManager;

    @Before
    public void setUp() {
        mActivityScenarioRule.getScenario().onActivity(this::onActivity);
    }

    private void onActivity(ChromeTabbedActivity activity) {
        // Spy so showSnackbar() is a no-op: these tests exercise only the dedup bookkeeping, not
        // the real snackbar view inflation/animation.
        mManager =
                spy(
                        new BraveSnackbarManager(
                                activity,
                                new FrameLayout(activity),
                                /* windowAndroid= */ null,
                                /* additionalBottomMarginPxSupplier= */ null,
                                /* modalDialogManager= */ null));
        doNothing().when(mManager).showSnackbar(any());
    }

    private static Snackbar makeNotice() {
        return Snackbar.make(
                        "New Tab Takeover",
                        new SnackbarController() {},
                        Snackbar.TYPE_PERSISTENT,
                        Snackbar.UMA_UNKNOWN)
                .setAction("Learn more", /* actionData= */ null);
    }

    @Test
    public void noNoticeOutstandingInitially() {
        assertFalse(mManager.hasNewTabTakeoverInfobar());
    }

    @Test
    public void showMarksNoticeOutstandingAndDisplaysIt() {
        Snackbar snackbar = makeNotice();
        mManager.showNewTabTakeoverInfobar(snackbar);

        assertTrue(mManager.hasNewTabTakeoverInfobar());
        verify(mManager).showSnackbar(snackbar);
    }

    @Test
    public void clearReleasesOutstandingNotice() {
        mManager.showNewTabTakeoverInfobar(makeNotice());
        assertTrue(mManager.hasNewTabTakeoverInfobar());

        mManager.clearNewTabTakeoverInfobar();
        assertFalse(mManager.hasNewTabTakeoverInfobar());
    }
}

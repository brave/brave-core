/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.Context;
import android.os.PowerManager;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Shadows;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.TabHidingType;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveFullscreenHtmlApiHandlerBaseTest {
    private static final class TestBraveFullscreenHtmlApiHandlerBase
            extends BraveFullscreenHtmlApiHandlerBase {}

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Activity mActivity;
    @Mock private BraveActivity mBraveActivity;

    private final TestBraveFullscreenHtmlApiHandlerBase mHandler =
            new TestBraveFullscreenHtmlApiHandlerBase();

    private PowerManager mPowerManager;
    private KeyguardManager mKeyguardManager;

    @Before
    public void setUp() {
        Context context = ContextUtils.getApplicationContext();
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mKeyguardManager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
    }

    @Test
    @CommandLineFlags.Add("disable-background-media-suspend")
    public void generalPictureInPicture_screenOff_preservesFullscreen() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add("disable-background-media-suspend")
    public void generalPictureInPicture_locked_preservesFullscreenUntilUnlocked() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));

        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add("disable-background-media-suspend")
    public void nonPictureInPicture_screenOffAndLocked_doesNotPreserveFullscreen() {
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Remove("disable-background-media-suspend")
    public void generalPictureInPicture_backgroundPlaybackDisabledScreenOff_doesNotPreserve() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Remove("disable-background-media-suspend")
    public void generalPictureInPicture_backgroundPlaybackDisabledLocked_doesNotPreserve() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add("disable-background-media-suspend")
    public void tabHidden_lockedPictureInPicture_recordsAndClearsTabSwitch() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertTrue(
                mHandler.maybeSkipExitFullscreenOnTabHidden(mActivity, TabHidingType.CHANGED_TABS));
        assertTrue(mHandler.mTabHiddenByChangedTabs);
        assertTrue(
                mHandler.maybeSkipExitFullscreenOnTabHidden(
                        mActivity, TabHidingType.ACTIVITY_HIDDEN));
        assertFalse(mHandler.mTabHiddenByChangedTabs);
    }

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_nonBraveActivity() {
        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_inactiveBraveActivity() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mBraveActivity));
    }

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_activeBraveActivity() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mBraveActivity));
    }
}

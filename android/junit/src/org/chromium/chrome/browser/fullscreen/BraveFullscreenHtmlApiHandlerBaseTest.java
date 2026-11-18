/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.PictureInPictureParams;
import android.content.Context;
import android.os.PowerManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.Shadows;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.media.YouTubePictureInPictureHost;
import org.chromium.chrome.browser.tab.TabHidingType;
import org.chromium.content_public.browser.WebContents;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveFullscreenHtmlApiHandlerBaseTest {
    private static final String DISABLE_BACKGROUND_MEDIA_SUSPEND =
            "disable-background-media-suspend";

    private static final class TestBraveFullscreenHtmlApiHandlerBase
            extends BraveFullscreenHtmlApiHandlerBase {}

    /** Stands in for BraveActivity, which is too heavy to build in a unit test. */
    public static final class TestHostActivity extends Activity
            implements YouTubePictureInPictureHost {
        private boolean mYouTubePictureInPictureActive;

        @Override
        public void onYouTubePictureInPictureRequested(WebContents webContents) {}

        @Override
        public void onYouTubePictureInPictureEnterFailed() {}

        @Override
        public boolean isYouTubePictureInPictureActive() {
            return mYouTubePictureInPictureActive;
        }

        @Override
        public void onYouTubePictureInPictureFullscreenInterrupted() {}

        @Override
        public void onYouTubePictureInPictureNewTab() {}
    }

    private Activity mActivity;
    private TestHostActivity mHostActivity;

    private final TestBraveFullscreenHtmlApiHandlerBase mHandler =
            new TestBraveFullscreenHtmlApiHandlerBase();

    private PowerManager mPowerManager;
    private KeyguardManager mKeyguardManager;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        mHostActivity = Robolectric.buildActivity(TestHostActivity.class).setup().get();
        Context context = ContextUtils.getApplicationContext();
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mKeyguardManager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
    }

    private static void enterPictureInPicture(Activity activity) {
        assertTrue(
                activity.enterPictureInPictureMode(new PictureInPictureParams.Builder().build()));
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_screenOff_preservesFullscreen() {
        enterPictureInPicture(mActivity);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_locked_preservesFullscreenUntilUnlocked() {
        enterPictureInPicture(mActivity);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));

        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void nonPictureInPicture_screenOffAndLocked_doesNotPreserveFullscreen() {
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Remove(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_backgroundPlaybackDisabledScreenOff_doesNotPreserve() {
        enterPictureInPicture(mActivity);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Remove(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_backgroundPlaybackDisabledLocked_doesNotPreserve() {
        enterPictureInPicture(mActivity);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void tabHidden_lockedPictureInPicture_recordsAndClearsTabSwitch() {
        enterPictureInPicture(mActivity);
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
        mHostActivity.mYouTubePictureInPictureActive = false;

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mHostActivity));
    }

    @Test
    public void shouldPreservePersistentFullscreenForPictureInPicture_activeBraveActivity() {
        mHostActivity.mYouTubePictureInPictureActive = true;

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mHostActivity));
    }
}

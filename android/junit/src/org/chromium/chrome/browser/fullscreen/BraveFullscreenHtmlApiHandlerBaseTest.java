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

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.Shadows;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.TabHidingType;
import org.chromium.chrome.browser.ui.ExclusiveAccessBubble;
import org.chromium.chrome.browser.ui.ExclusiveAccessContext;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveFullscreenHtmlApiHandlerBaseTest {
    private static final String DISABLE_BACKGROUND_MEDIA_SUSPEND =
            "disable-background-media-suspend";

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
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_SHOW_FULLSCREEN_NOTICES);
        Context context = ContextUtils.getApplicationContext();
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mKeyguardManager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_screenOff_preservesFullscreen() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        assertTrue(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_locked_preservesFullscreenUntilUnlocked() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
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
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Remove(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void generalPictureInPicture_backgroundPlaybackDisabledLocked_doesNotPreserve() {
        when(mActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        assertFalse(mHandler.shouldPreservePersistentFullscreenForPictureInPicture(mActivity));
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
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

    @After
    public void clearFullscreenNoticePreference() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_SHOW_FULLSCREEN_NOTICES);
    }

    @Test
    public void legacyToast_readsPreferenceOnEachDisplay() {
        Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
        FullscreenToast toast = new FullscreenToast.AndroidToast(activity, () -> true);
        try {
            toast.onFullscreenLayout();
            assertTrue(toast.isVisible());

            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_FULLSCREEN_NOTICES, false);
            toast.onWindowFocusChanged(true);
            assertFalse(toast.isVisible());
            toast.onFullscreenLayout();
            assertFalse(toast.isVisible());

            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_FULLSCREEN_NOTICES, true);
            toast.onWindowFocusChanged(true);
            assertTrue(toast.isVisible());
        } finally {
            toast.onExitFullscreen();
            activity.finish();
        }
    }

    @Test
    public void nativeBubblePreference_readsCurrentValueWithoutCaching() {
        ExclusiveAccessBubble bubble =
                ExclusiveAccessBubble.create(
                        org.mockito.Mockito.mock(ExclusiveAccessContext.class));
        assertTrue(bubble.shouldShowFullscreenNotice());
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_FULLSCREEN_NOTICES, false);
        assertFalse(bubble.shouldShowFullscreenNotice());
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_FULLSCREEN_NOTICES, true);
        assertTrue(bubble.shouldShowFullscreenNotice());
    }
}

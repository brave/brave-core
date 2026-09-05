/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.IntentUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeInactivityTracker;
import org.chromium.chrome.browser.ntp.BraveFreshNtpHelper;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.browser_ui.media.MediaNotificationController;
import org.chromium.components.browser_ui.media.MediaNotificationInfo;
import org.chromium.components.browser_ui.media.MediaNotificationListener;
import org.chromium.components.browser_ui.media.MediaNotificationManager;
import org.chromium.services.media_session.MediaMetadata;

/** Unit tests for {@link BraveReturnToChromeUtil} class. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveReturnToChromeUtilUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock private ChromeInactivityTracker mInactivityTracker;

    @Test
    @SmallTest
    public void testShouldNotShowNtpWithExternalViewIntent() {
        // Simulate an ACTION_VIEW intent from an external app (e.g., clicking a link in Gmail).
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://example.com"));

        assertFalse(IntentUtils.isMainIntentFromLauncher(intent));
        assertFalse(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        intent, null, /* persistableBundle= */ null, mInactivityTracker));

        // Verify snackbar flag was NOT set.
        assertFalse(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, false));
    }

    @Test
    @SmallTest
    public void testShouldNotShowNtpWithSendIntent() {
        // Simulate an ACTION_SEND intent (e.g., sharing a link to Brave).
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.putExtra(Intent.EXTRA_TEXT, "https://example.com");

        assertFalse(IntentUtils.isMainIntentFromLauncher(intent));
        assertFalse(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        intent, null, /* persistableBundle= */ null, mInactivityTracker));
    }

    @Test
    @SmallTest
    public void testMainLauncherIntentIsRecognized() {
        // Verify that a proper main launcher intent passes the intent check.
        Intent intent = createMainIntentFromLauncher();
        assertTrue(IntentUtils.isMainIntentFromLauncher(intent));
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT)
    public void testShouldShowNtpAfterInactivityWhenNoMediaPlaying() {
        setUpInactivityVariant();
        MediaNotificationManager.hideForAllTabs(R.id.media_playback_notification);
        MediaNotificationManager.setService(R.id.media_playback_notification, null);

        assertTrue(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        createMainIntentFromLauncher(),
                        null,
                        /* persistableBundle= */ null,
                        mInactivityTracker));
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT)
    public void testShouldNotShowNtpAfterInactivityWhenMediaPlaying() {
        setUpInactivityVariant();
        setMediaControllerPaused(false);

        assertFalse(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        createMainIntentFromLauncher(),
                        null,
                        /* persistableBundle= */ null,
                        mInactivityTracker));

        // Verify snackbar flag was NOT set while media is playing.
        assertFalse(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, false));
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT)
    public void testShouldShowNtpAfterInactivityWhenMediaPaused() {
        setUpInactivityVariant();
        // A paused media notification means the user is not actively listening, so the NTP
        // should still be shown after inactivity.
        setMediaControllerPaused(true);

        assertTrue(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        createMainIntentFromLauncher(),
                        null,
                        /* persistableBundle= */ null,
                        mInactivityTracker));
    }

    private Intent createMainIntentFromLauncher() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        return intent;
    }

    /**
     * Configures variant B (1-hour threshold) with the "new tab after inactivity" opening screen
     * option and a background duration that comfortably exceeds the threshold.
     */
    private void setUpInactivityVariant() {
        BraveFreshNtpHelper.sBraveFreshNtpAfterIdleExperimentVariant.setForTesting("B");
        ChromeSharedPreferences.getInstance()
                .writeInt(
                        BravePreferenceKeys.BRAVE_NEW_TAB_PAGE_OPENING_SCREEN,
                        BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB_AFTER_INACTIVITY);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, false);
        // Two hours, well beyond variant B's 1-hour threshold.
        when(mInactivityTracker.getTimeSinceLastBackgroundedMs()).thenReturn(2 * 60 * 60 * 1000L);
    }

    /** Registers a media playback notification controller in the given paused state. */
    private void setMediaControllerPaused(boolean isPaused) {
        MediaNotificationInfo info =
                new MediaNotificationInfo.Builder()
                        .setMetadata(new MediaMetadata("title", "artist", "album"))
                        .setOrigin("https://example.com")
                        .setListener(mock(MediaNotificationListener.class))
                        .setInstanceId(1)
                        .setId(R.id.media_playback_notification)
                        .setPaused(isPaused)
                        .build();
        MediaNotificationController controller = mock(MediaNotificationController.class);
        controller.mMediaNotificationInfo = info;
        MediaNotificationManager.setControllerForTesting(
                R.id.media_playback_notification, controller);
    }
}

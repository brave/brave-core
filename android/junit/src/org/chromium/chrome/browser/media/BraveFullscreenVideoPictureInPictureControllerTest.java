/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.Context;
import android.os.PowerManager;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Shadows;
import org.robolectric.shadows.ShadowSystemClock;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.content_public.browser.test.mock.MockWebContents;
import org.chromium.media_session.mojom.MediaSession.SuspendType;

import java.time.Duration;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveFullscreenVideoPictureInPictureControllerTest {
    private static final String DISABLE_BACKGROUND_MEDIA_SUSPEND =
            "disable-background-media-suspend";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;
    @Mock private FullscreenManager mFullscreenManager;
    @Mock private Tab mTab;
    @Mock private MockWebContents mWebContents;
    @Mock private MediaSession mMediaSession;
    private WebContentsObserver mObserver;
    private PowerManager mPowerManager;
    private KeyguardManager mKeyguardManager;

    // Creates a playing PiP controller through the upstream lifecycle.
    private FullscreenVideoPictureInPictureController enterPlayingPictureInPicture(
            Activity activity) {
        Context context = ContextUtils.getApplicationContext();
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mKeyguardManager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
        when(activity.getSystemService(Context.POWER_SERVICE)).thenReturn(mPowerManager);
        when(mTab.getWebContents()).thenReturn(mWebContents);
        ActivityTabProvider activityTabProvider = new ActivityTabProvider();
        activityTabProvider.setForTesting(mTab);
        FullscreenVideoPictureInPictureController controller =
                new FullscreenVideoPictureInPictureController(
                        activity, activityTabProvider, mFullscreenManager) {
                    @Override
                    MediaSession getMediaSession() {
                        return mMediaSession;
                    }

                    @Override
                    void assertLibraryLoaderIsInitialized() {}
                };
        controller.onEnteredPictureInPictureMode();
        ArgumentCaptor<WebContentsObserver> observer =
                ArgumentCaptor.forClass(WebContentsObserver.class);
        verify(mWebContents).addObserver(observer.capture());
        mObserver = observer.getValue();
        mObserver.mediaStartedPlaying(0, /* hasAudio= */ true, /* hasVideo= */ true);
        // Let a real close run immediately rather than defer past the assertions.
        ShadowSystemClock.advanceBy(
                Duration.ofMillis(FullscreenVideoPictureInPictureController.MIN_EXIT_DELAY_MILLIS));
        return controller;
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onStop_screenOff_preservesPlaybackUntilAwakeClose() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        controller.onStop();
        verify(mMediaSession, never()).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession, never()).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);

        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        controller.onStart();
        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onStop_nonBraveActivityLocked_preservesPlaybackUntilUnlockedClose() {
        Activity activity = mock(Activity.class);
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(activity);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        controller.onStop();
        verify(mMediaSession, never()).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession, never()).suspend(SuspendType.SYSTEM);

        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
        controller.onStart();
        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onStop_awakeClose_suspendsOnce() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Remove(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onStop_backgroundPlaybackDisabledScreenOff_suspendsOnce() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Remove(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onStop_backgroundPlaybackDisabledLocked_suspendsOnce() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onResume_afterRepeatedScreenLocks_keepsPictureInPictureUntilClose() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);

        for (int i = 0; i < 2; i++) {
            Shadows.shadowOf(mPowerManager).setIsInteractive(false);
            Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);
            controller.onStop();
            Shadows.shadowOf(mPowerManager).setIsInteractive(true);
            Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
            controller.onStart();
            controller.onResume();
        }

        verify(mBraveActivity, never()).moveTaskToBack(/* nonRoot= */ true);
        verify(mMediaSession, never()).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);

        controller.onStop();
        verify(mMediaSession).suspend(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onResume_userPausedBeforeScreenLock_keepsPictureInPicturePaused() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        mObserver.mediaStoppedPlaying(0);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(true);
        controller.onStop();

        // Waking the display alone does not unlock the phone.
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        controller.onStart();
        controller.onStop();
        Shadows.shadowOf(mKeyguardManager).setKeyguardLocked(false);
        controller.onStart();
        controller.onResume();
        controller.onStop();

        verify(mBraveActivity, never()).moveTaskToBack(/* nonRoot= */ true);
        verify(mMediaSession, never()).suspend(SuspendType.SYSTEM);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onResume_withoutScreenLock_preservesUpstreamDismiss() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);

        controller.onResume();

        verify(mBraveActivity).moveTaskToBack(/* nonRoot= */ true);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onResume_afterUnlockThenOrdinaryResume_dismissesPictureInPicture() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);
        controller.onStop();
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);
        controller.onResume();
        verify(mBraveActivity, never()).moveTaskToBack(/* nonRoot= */ true);

        controller.onResume();

        verify(mBraveActivity).moveTaskToBack(/* nonRoot= */ true);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    @Test
    @CommandLineFlags.Add(DISABLE_BACKGROUND_MEDIA_SUSPEND)
    public void onResume_leftPictureInPictureAfterScreenLock_preservesUpstreamDismiss() {
        FullscreenVideoPictureInPictureController controller =
                enterPlayingPictureInPicture(mBraveActivity);
        Shadows.shadowOf(mPowerManager).setIsInteractive(false);
        controller.onStop();
        Shadows.shadowOf(mPowerManager).setIsInteractive(true);

        controller.onResume();

        verify(mBraveActivity).moveTaskToBack(/* nonRoot= */ true);
        verify(mMediaSession, never()).resume(SuspendType.SYSTEM);
    }

    private final BraveFullscreenVideoPictureInPictureController mController =
            new BraveFullscreenVideoPictureInPictureController();

    @Test
    public void maybeHandleDismiss_activeYouTubePictureInPictureDefersResumeCleanup() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ true,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_activeYouTubePictureInPictureDefersStartCleanup() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ true,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_activePictureInPictureKeepsPictureInPictureAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ true,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_webContentsSignalDuringActivePictureInPicture_keepsAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ true,
                        /* isNewTab= */ false);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_inactiveYouTubeAndNotInPip_doesNotKeepAlive() {
        // Not a YT PiP session and not in PiP: the hook should fall through to the upstream
        // dismiss logic.
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(false);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ true,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ false);

        assertFalse(handled);
        assertTrue(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_newTabDuringActiveYouTubePictureInPicture_keepsForeground() {
        // A new tab arriving during a Brave-managed YouTube PiP session must skip upstream's
        // moveTaskToBack(true) so the activity stays in the foreground (e.g. when the user taps
        // the "New tab" launcher shortcut while PiP is active). The hook also signals the
        // controller to drop persistent fullscreen so the new tab is rendered correctly.
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ true);

        assertTrue(handled);
        assertFalse(mController.mDismissPending);
        verify(mBraveActivity).onYouTubePictureInPictureNewTab();
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_newTabWhenActiveButNotInPictureInPicture_doesNotKeepAlive() {
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(false);
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(true);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ true);

        assertFalse(handled);
        assertTrue(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureNewTab();
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }

    @Test
    public void maybeHandleDismiss_newTabWithoutActiveYouTubePictureInPicture_doesNotKeepAlive() {
        when(mBraveActivity.isYouTubePictureInPictureActive()).thenReturn(false);
        mController.mDismissPending = true;

        boolean handled =
                mController.maybeHandleDismissActivityForYouTubePictureInPicture(
                        mBraveActivity,
                        /* isStart= */ false,
                        /* isResume= */ false,
                        /* isLeftFullscreen= */ false,
                        /* isWebContentsLeftFullscreen= */ false,
                        /* isNewTab= */ true);

        assertFalse(handled);
        assertTrue(mController.mDismissPending);
        verify(mBraveActivity, never()).onYouTubePictureInPictureNewTab();
        verify(mBraveActivity, never()).onYouTubePictureInPictureFullscreenInterrupted();
    }
}

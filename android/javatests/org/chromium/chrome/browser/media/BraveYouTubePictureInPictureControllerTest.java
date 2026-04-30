/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.Bundle;
import android.os.PowerManager;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.fullscreen.BraveFullscreenHtmlApiHandlerBase;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.WebContents;

/**
 * Unit tests for {@link BraveYouTubePictureInPictureController}'s pure state-transition surface.
 */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveYouTubePictureInPictureControllerTest {
    /**
     * The bytecode-injected upstream {@code FullscreenHtmlApiHandlerBase} extends Brave's abstract
     * handler and implements {@code FullscreenManager}. This stub captures both relationships so a
     * single Mockito mock can satisfy the controller's {@code fullscreenManager instanceof
     * BraveFullscreenHtmlApiHandlerBase} check.
     */
    private abstract static class FullscreenHandlerStub extends BraveFullscreenHtmlApiHandlerBase
            implements FullscreenManager {}

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;
    @Mock private WebContents mWebContents;
    @Mock private WebContents mOtherWebContents;
    @Mock private FullscreenManager mFullscreenManager;
    @Mock private FullscreenHandlerStub mBraveFullscreenHandler;
    @Mock private PowerManager mPowerManager;
    @Mock private MediaSession mMediaSession;

    @After
    public void tearDown() {
        // The controller writes the active session into a process-wide registry. Reset it so
        // tests don't bleed state into each other.
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(null);
    }

    @Test
    @SmallTest
    public void initiallyInactive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        assertFalse(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onSessionRequested_marksActive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onSessionRequested(mWebContents);

        assertTrue(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onSessionEnterFailed_clearsActive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        controller.onSessionEnterFailed();

        assertFalse(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onFullscreenInterrupted_whenInactive_isNoop() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onFullscreenInterrupted();

        assertFalse(controller.isActive());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void saveAndRestore_preservesActive() {
        BraveYouTubePictureInPictureController source =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        source.onSessionRequested(mWebContents);

        Bundle out = new Bundle();
        source.onSaveInstanceState(out);

        BraveYouTubePictureInPictureController restored =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        restored.onPostCreate(out);

        assertTrue(restored.isActive());
        // Web contents reference is intentionally not persisted; recovery picks up from the
        // current activity tab on the next visible-after-screen-on tick.
        assertFalse(restored.isExitingForTesting());
    }

    @Test
    @SmallTest
    public void onPostCreate_withInterruptedActiveSession_registersScreenStateReceiver() {
        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putBoolean(
                BraveYouTubePictureInPictureController.KEY_INTERRUPTED_BY_SCREEN_LOCK, true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        assertTrue(controller.isActive());
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        assertTrue(controller.hasScreenStateReceiverForTesting());
    }

    @Test
    @SmallTest
    public void onPostCreate_withNullBundle_initializesInactive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onPostCreate(null);

        assertFalse(controller.isActive());
    }

    @Test
    @SmallTest
    public void onNewTabDuringPictureInPicture_inactive_isNoop() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onNewTabDuringPictureInPicture();

        assertFalse(controller.isActive());
        // Confirms the early-return short-circuit fires before any teardown work.
        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mWebContents, never()).exitFullscreen();
    }

    @Test
    @SmallTest
    public void onNewTabDuringPictureInPicture_screenLocked_marksInterruptedAndPreservesSession() {
        when(mBraveActivity.getSystemService(Context.POWER_SERVICE)).thenReturn(mPowerManager);
        when(mPowerManager.isInteractive()).thenReturn(false);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        try (MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();
        }

        // Session preserved for resume on unlock; no teardown was performed and playback is
        // not paused (the screen-state receiver decides that on unlock).
        assertTrue(controller.isActive());
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mWebContents, never()).exitFullscreen();
        verify(mMediaSession, never()).suspend();
    }

    @Test
    @SmallTest
    public void onNewTabDuringPictureInPicture_active_suspendsExitsAndClearsSession() {
        when(mBraveActivity.getFullscreenManager()).thenReturn(mFullscreenManager);
        when(mFullscreenManager.getPersistentFullscreenMode()).thenReturn(false);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            // YouTube-side player exit is requested via the JS helper.
            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));
        }

        // YouTube playback is paused so audio doesn't continue while the user is on the new tab.
        verify(mMediaSession).suspend();
        // WebContents-level exit is invoked directly on the tracked tab so the renderer drops
        // DOM fullscreen even when FullscreenManager.mWebContentsInFullscreen is stale.
        verify(mWebContents).exitFullscreen();
        assertFalse(controller.isActive());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    @SmallTest
    public void onNewTabDuringPictureInPicture_restoredSession_usesRegisteredWebContents() {
        when(mBraveActivity.getFullscreenManager()).thenReturn(mFullscreenManager);
        when(mFullscreenManager.getPersistentFullscreenMode()).thenReturn(false);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mWebContents);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                mockStatic(BraveYouTubeScriptInjectorNativeHelper.class)) {
            controller.onNewTabDuringPictureInPicture();

            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));
            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mOtherWebContents),
                    never());
        }

        verify(mBraveActivity, never()).getCurrentWebContents();
        verify(mWebContents).exitFullscreen();
        verify(mOtherWebContents, never()).exitFullscreen();
        assertFalse(controller.isActive());
    }

    @Test
    @SmallTest
    public void onNewTabDuringPictureInPicture_persistentFullscreen_exitsBraveUi() {
        when(mBraveActivity.getFullscreenManager()).thenReturn(mBraveFullscreenHandler);
        when(mBraveFullscreenHandler.getPersistentFullscreenMode()).thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));
        }

        // Brave's PiP-specific persistent-fullscreen exit is invoked synchronously so the new
        // tab is rendered with browser chrome rather than under a residual fullscreen layout.
        verify(mBraveFullscreenHandler).exitPersistentFullscreenModeForPictureInPicture();
        verify(mMediaSession).suspend();
        verify(mWebContents).exitFullscreen();
        assertFalse(controller.isActive());
    }
}

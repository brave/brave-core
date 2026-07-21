/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.app.KeyguardManager;
import android.app.PictureInPictureParams;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowKeyguardManager;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelperJni;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.MediaSessionObserver;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.media_session.mojom.MediaSession.SuspendType;

import java.util.concurrent.TimeUnit;

/** Unit tests for {@link BraveYouTubePictureInPictureController}'s state-transition surface. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(shadows = {ShadowLooper.class})
public class BraveYouTubePictureInPictureControllerTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;
    @Mock private WebContents mWebContents;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private FullscreenManager mFullscreenManager;
    @Mock private BraveYouTubeScriptInjectorNativeHelper.Natives mNatives;
    @Mock private MediaSession mMediaSession;

    @Before
    public void setUp() {
        BraveYouTubeScriptInjectorNativeHelperJni.setInstanceForTesting(mNatives);
    }

    @After
    public void tearDown() {
        // The controller writes the active session into a process-wide registry. Reset it so
        // tests don't bleed state into each other.
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(null);
    }

    @Test
    public void initiallyInactive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        assertFalse(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    public void onSessionRequested_marksActive() {
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(controller).getMediaSession(any());

        controller.onSessionRequested(mWebContents);

        assertTrue(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    public void onSessionEnterFailed_clearsActive() {
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(controller).getMediaSession(any());
        controller.onSessionRequested(mWebContents);

        controller.onSessionEnterFailed();

        assertFalse(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    public void onFullscreenInterrupted_whenInactive_isNoop() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onFullscreenInterrupted();

        assertFalse(controller.isActive());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    public void saveAndRestore_preservesActive() {
        BraveYouTubePictureInPictureController source =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(source).getMediaSession(any());
        source.onSessionRequested(mWebContents);

        Bundle out = new Bundle();
        source.onSaveInstanceState(out);

        // The restore validates against ground truth: model a recreation while the PiP window
        // is up, the case where carrying the session over is correct.
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        BraveYouTubePictureInPictureController restored =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        restored.onPostCreate(out);

        assertTrue(restored.isActive());
        // Web contents reference is intentionally not persisted; recovery picks up from the
        // current activity tab on the next visible-after-screen-on tick.
        assertFalse(restored.isExitingForTesting());
    }

    @Test
    public void onPostCreate_withInterruptedActiveSession_restoresAndArmsReceiverOnResume() {
        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putBoolean(
                BraveYouTubePictureInPictureController.KEY_INTERRUPTED_BY_SCREEN_LOCK, true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        // The session is restored, but the receiver is deliberately NOT registered yet:
        // onPostCreate runs pre-native, and a broadcast delivered in that window would reach
        // code that needs the tab models.
        assertTrue(controller.isActive());
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        assertFalse(controller.hasScreenStateReceiverForTesting());

        // onResume (native ready) arms the receiver; the device is still locked, so the
        // session keeps waiting for the unlock broadcast.
        setKeyguardLocked();
        controller.onResume();

        assertTrue(controller.isActive());
        assertTrue(controller.hasScreenStateReceiverForTesting());
    }

    @Test
    public void onPostCreate_staleActiveSession_isDiscarded() {
        // A bundle can claim an active session while no PiP window survived the recreation and
        // no lock interruption justifies it (process death, PiP dismissed in the recreation
        // gap). Restoring it would latch the session active for the activity lifetime, since
        // no framework callback would ever end it.
        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        assertFalse(controller.isActive());
    }

    @Test
    public void onPostCreate_activeInPictureInPicture_isRestored() {
        // Recreation while the PiP window is up (e.g. a configuration change): the window
        // survives, so the session must be restored even without a lock interruption.
        when(mBraveActivity.isInPictureInPictureMode()).thenReturn(true);
        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        assertTrue(controller.isActive());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    public void onPostCreate_withNullBundle_initializesInactive() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onPostCreate(null);

        assertFalse(controller.isActive());
    }

    @Test
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
    public void onNewTabDuringPictureInPicture_screenLocked_marksInterruptedAndPreservesSession() {
        setKeyguardLocked();

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(false).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doReturn(null).when(controller).getMediaSession(any());
        doNothing().when(controller).suspendMediaSession(any());
        controller.onSessionRequested(mWebContents);

        controller.onNewTabDuringPictureInPicture();

        // Session preserved for resume on unlock; no teardown was performed. Playback is paused
        // for the locked gap because background video playback is disabled.
        assertTrue(controller.isActive());
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mWebContents, never()).exitFullscreen();
        verify(controller).suspendMediaSession(mWebContents);
    }

    @Test
    public void onFullscreenInterrupted_backgroundPlayDisabled_pausesPlayback() {
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(false).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doReturn(null).when(controller).getMediaSession(any());
        doNothing().when(controller).suspendMediaSession(any());
        controller.onSessionRequested(mWebContents);

        controller.onFullscreenInterrupted();

        // The user has not opted into background playback, so the screen-lock interruption must
        // pause the video.
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        verify(controller).suspendMediaSession(mWebContents);
    }

    @Test
    public void onFullscreenInterrupted_backgroundPlayEnabled_keepsPlaying() {
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(true).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doReturn(null).when(controller).getMediaSession(any());
        controller.onSessionRequested(mWebContents);

        controller.onFullscreenInterrupted();

        // The user opted into background playback, so audio keeps playing while the device is
        // locked and the session waits for the screen-state receiver to restore PiP.
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        verify(controller, never()).suspendMediaSession(any());
    }

    @Test
    public void resumeAfterScreenLock_backgroundPlayDisabled_reentersPipWithoutResumingMedia() {
        // The video was paused for the locked gap. The PiP re-entry after unlock must not
        // schedule a media-session resume: YouTube videos do not auto-resume after unlock
        // outside PiP either, so the session re-enters paused and the user resumes playback
        // from the PiP window controls.
        when(mNatives.isPictureInPictureAvailable(mWebContents)).thenReturn(true);
        when(mBraveActivity.enterPictureInPictureMode(any(PictureInPictureParams.class)))
                .thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(false).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doReturn(mMediaSession).when(controller).getMediaSession(any());
        doNothing().when(controller).suspendMediaSession(any());
        controller.onSessionRequested(mWebContents);
        // Media is playing when the lock hits; the bundle assertion below must prove the
        // background-play gate, not an incidental "nothing was playing" default.
        captureMediaSessionObserver()
                .mediaSessionStateChanged(/* isControllable= */ true, /* isSuspended= */ false);
        controller.onFullscreenInterrupted();

        // Screen is unlocked by default in this environment, so this runs the resume path.
        controller.onResume();

        assertFalse(controller.isInterruptedByScreenLockForTesting());
        Bundle outState = new Bundle();
        controller.onSaveInstanceState(outState);
        assertFalse(
                outState.getBoolean(
                        BraveYouTubePictureInPictureController
                                .KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY,
                        true));
    }

    @Test
    public void resumeAfterScreenLock_backgroundPlayEnabled_schedulesMediaResumeOnPipEntry() {
        when(mNatives.isPictureInPictureAvailable(mWebContents)).thenReturn(true);
        when(mBraveActivity.enterPictureInPictureMode(any(PictureInPictureParams.class)))
                .thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(true).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doReturn(mMediaSession).when(controller).getMediaSession(any());
        controller.onSessionRequested(mWebContents);
        // Media is playing when the lock hits.
        captureMediaSessionObserver()
                .mediaSessionStateChanged(/* isControllable= */ true, /* isSuspended= */ false);
        controller.onFullscreenInterrupted();

        controller.onResume();

        // Audio kept playing while locked; the deferred resume stays scheduled so
        // onEnterPictureInPictureMode() can counteract any system-initiated pause.
        assertFalse(controller.isInterruptedByScreenLockForTesting());
        Bundle outState = new Bundle();
        controller.onSaveInstanceState(outState);
        assertTrue(
                outState.getBoolean(
                        BraveYouTubePictureInPictureController
                                .KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY,
                        false));
    }

    @Test
    public void resumeAfterScreenLock_userPausedBeforeLock_reentersPipWithoutResumingMedia() {
        // The user paused the video before locking the device, so even with background playback
        // enabled the PiP re-entry must not schedule a resume: it would force playback the user
        // did not ask for.
        when(mNatives.isPictureInPictureAvailable(mWebContents)).thenReturn(true);
        when(mBraveActivity.enterPictureInPictureMode(any(PictureInPictureParams.class)))
                .thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(true).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doReturn(mMediaSession).when(controller).getMediaSession(any());
        controller.onSessionRequested(mWebContents);
        // The media session last reported itself paused: the user paused before locking.
        captureMediaSessionObserver()
                .mediaSessionStateChanged(/* isControllable= */ true, /* isSuspended= */ true);
        controller.onFullscreenInterrupted();

        controller.onResume();

        assertFalse(controller.isInterruptedByScreenLockForTesting());
        Bundle outState = new Bundle();
        controller.onSaveInstanceState(outState);
        assertFalse(
                outState.getBoolean(
                        BraveYouTubePictureInPictureController
                                .KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY,
                        true));
    }

    /** Returns the play/pause observer the controller attached to {@code mMediaSession}. */
    private MediaSessionObserver captureMediaSessionObserver() {
        ArgumentCaptor<MediaSessionObserver> captor =
                ArgumentCaptor.forClass(MediaSessionObserver.class);
        verify(mMediaSession).addObserver(captor.capture());
        return captor.getValue();
    }

    @Test
    public void onDestroy_withoutSession_doesNotWipeRegistry() {
        // Every BraveActivity lazily creates a controller on its destroy path; a session-less
        // window (multi-window) must not clear another window's live registry entry.
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mWebContents);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onDestroy();

        assertSame(mWebContents, BraveMediaSessionHelper.getYouTubePictureInPictureWebContents());
    }

    @Test
    public void onEnterPictureInPictureMode_resumesSessionWebContents() {
        // The entry resume must target the session's own WebContents: after a screen-lock
        // re-entry the foreground tab can differ from the session tab, and resuming the
        // foreground tab would start unrelated media while the PiP video stays paused.
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(mMediaSession).when(controller).getMediaSession(any());
        controller.onSessionRequested(mWebContents);

        assertTrue(controller.onEnterPictureInPictureMode());

        verify(mMediaSession).resume(SuspendType.SYSTEM);
        verify(mBraveActivity, never()).getCurrentWebContents();
    }

    @Test
    public void screenOffBroadcast_pausesPlaybackDuringActiveSession() {
        // SCREEN_OFF is the only reliable lock signal when the PiP window survives the lock:
        // Android keeps the activity in PiP mode and the still-playing media produces no
        // upstream fullscreen-left signal, so the receiver must drive the pause directly.
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(controller).getMediaSession(any());
        doReturn(false).when(controller).isBackgroundVideoPlaybackEnabled(any());
        doNothing().when(controller).suspendMediaSession(any());

        controller.onSessionRequested(mWebContents);
        // The receiver is armed for the whole session, not only after an interruption.
        assertTrue(controller.hasScreenStateReceiverForTesting());

        controller
                .getScreenStateReceiverForTesting()
                .onReceive(
                        ContextUtils.getApplicationContext(), new Intent(Intent.ACTION_SCREEN_OFF));

        assertTrue(controller.isInterruptedByScreenLockForTesting());
        verify(controller).suspendMediaSession(mWebContents);
    }

    @Test
    public void screenOffBroadcast_inactiveSession_isNoop() {
        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(controller).getMediaSession(any());
        doNothing().when(controller).suspendMediaSession(any());
        controller.onSessionRequested(mWebContents);
        controller.onSessionEnterFailed();

        // clearSession tears the receiver down with the session.
        assertFalse(controller.hasScreenStateReceiverForTesting());
    }

    @Test
    public void onDestroy_restoredSessionWithoutWebContents_doesNotWipeRegistry() {
        // A restored session that never re-bound a WebContents must not clear the registry on
        // destroy: the slot may belong to another window's live session started while ours was
        // lock-interrupted.
        WebContents otherWindowWebContents = mock(WebContents.class);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(otherWindowWebContents);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putBoolean(
                BraveYouTubePictureInPictureController.KEY_INTERRUPTED_BY_SCREEN_LOCK, true);
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        controller.onDestroy();

        assertSame(
                otherWindowWebContents,
                BraveMediaSessionHelper.getYouTubePictureInPictureWebContents());
    }

    @Test
    public void clearSession_withoutWebContents_doesNotWipeRegistry() {
        // Same guard on the clearSession path: ending a session that never bound a WebContents
        // (here via a failed entry after restore) must leave a foreign registry entry alone.
        WebContents otherWindowWebContents = mock(WebContents.class);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(otherWindowWebContents);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putBoolean(
                BraveYouTubePictureInPictureController.KEY_INTERRUPTED_BY_SCREEN_LOCK, true);
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        controller.onSessionEnterFailed();

        assertFalse(controller.isActive());
        assertSame(
                otherWindowWebContents,
                BraveMediaSessionHelper.getYouTubePictureInPictureWebContents());
    }

    private static void setKeyguardLocked() {
        ShadowKeyguardManager shadowKeyguardManager =
                Shadows.shadowOf(
                        (KeyguardManager)
                                ContextUtils.getApplicationContext()
                                        .getSystemService(Context.KEYGUARD_SERVICE));
        shadowKeyguardManager.setKeyguardLocked(true);
    }

    @Test
    public void onNewTabDuringPictureInPicture_registryGcedAndTabGone_doesNotRetarget() {
        // Recovery fails cleanly when the tab id no longer resolves to a tab (for example after
        // the YouTube tab was closed during the activity-recreation window). The controller must
        // not fall back to the foreground tab which would silently retarget the session; the
        // operation simply no-ops on the renderer side.
        final int tabId = 42;
        when(mTabModelSelector.getTabById(tabId)).thenReturn(null);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putBoolean(
                BraveYouTubePictureInPictureController.KEY_INTERRUPTED_BY_SCREEN_LOCK, true);
        savedState.putInt(BraveYouTubePictureInPictureController.KEY_TAB_ID, tabId);

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(mTabModelSelector).when(controller).getTabModelSelectorIfReady();
        controller.onPostCreate(savedState);

        controller.onNewTabDuringPictureInPicture();

        verify(mBraveActivity, never()).getCurrentWebContents();
        verify(mWebContents, never()).exitFullscreen();
    }

    @Test
    public void onExitPictureInPictureMode_rendererFullscreenExit_runsCleanupOnce() {
        // The restore is event driven: as soon as the renderer reports DOM fullscreen has
        // exited, the controller must trigger maybeRestoreFullscreenUi without waiting for the
        // safety timer. The timer must then no op so the cleanup runs exactly once.
        WebContents observableWebContents =
                mock(
                        WebContents.class,
                        withSettings().extraInterfaces(WebContentsObserver.Observable.class));
        when(mBraveActivity.getFullscreenManager()).thenReturn(mFullscreenManager);
        when(mFullscreenManager.getPersistentFullscreenMode()).thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(controller).getMediaSession(any());
        controller.onSessionRequested(observableWebContents);

        controller.onExitPictureInPictureMode();

        // The DOM fullscreen exit was requested directly on the active session's WebContents.
        verify(observableWebContents).exitFullscreen();

        // Capture the observer the controller installed, then simulate the renderer
        // reporting DOM fullscreen exit.
        ArgumentCaptor<WebContentsObserver> observerCaptor =
                ArgumentCaptor.forClass(WebContentsObserver.class);
        verify((WebContentsObserver.Observable) observableWebContents)
                .addObserver(observerCaptor.capture());
        WebContentsObserver exitObserver = observerCaptor.getValue();
        exitObserver.didToggleFullscreenModeForTab(
                /* enteredFullscreen= */ false, /* willCauseResize= */ false);

        // Fast path fired: cleanup ran and the observer was detached. The restore also forces
        // the browsing layout in case the activity was recreated into the tab switcher while
        // the PiP window was up.
        verify(mFullscreenManager).exitPersistentFullscreenMode();
        verify(mBraveActivity).exitOverviewModeForYouTubePictureInPicture();
        verify((WebContentsObserver.Observable) observableWebContents).removeObserver(exitObserver);

        // Advance the looper just past the fallback delay so the safety task fires but the
        // delayed dismiss check stays queued (running it would touch MediaSession,
        // which has no JNI seam in this test environment). The fallback must short circuit
        // because the fast path already owned the cleanup.
        ShadowLooper.idleMainLooper(
                BraveYouTubePictureInPictureController.FULLSCREEN_EXIT_FALLBACK_MS,
                TimeUnit.MILLISECONDS);

        verify(mFullscreenManager, times(1)).exitPersistentFullscreenMode();
    }

    @Test
    public void maybeRestoreFullscreenUi_activityFinishing_skipsRestore() {
        // The fullscreen restore runs from delayed UI tasks (the renderer signal and the
        // FULLSCREEN_EXIT_FALLBACK_MS safety timer), which can fire after the activity has
        // started tearing down. When the activity is finishing/destroyed the restore must
        // return before touching getFullscreenManager(), so it never operates on torn-down
        // UI.
        WebContents observableWebContents =
                mock(
                        WebContents.class,
                        withSettings().extraInterfaces(WebContentsObserver.Observable.class));
        when(mBraveActivity.isActivityFinishingOrDestroyed()).thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                spy(new BraveYouTubePictureInPictureController(mBraveActivity));
        doReturn(null).when(controller).getMediaSession(any());
        controller.onSessionRequested(observableWebContents);

        controller.onExitPictureInPictureMode();

        // Let the safety timer fire (the renderer signal never arrives in this scenario). Without
        // the guard this delayed task would call getFullscreenManager() on the finishing activity.
        ShadowLooper.idleMainLooper(
                BraveYouTubePictureInPictureController.FULLSCREEN_EXIT_FALLBACK_MS,
                TimeUnit.MILLISECONDS);

        // Guard fired: the activity's fullscreen surface was never touched.
        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mFullscreenManager, never()).exitPersistentFullscreenMode();
        verify(mBraveActivity, never()).exitOverviewModeForYouTubePictureInPicture();
    }
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.app.KeyguardManager;
import android.content.Context;
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
import org.chromium.chrome.browser.fullscreen.BraveFullscreenHtmlApiHandlerBase;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelperJni;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

import java.util.concurrent.TimeUnit;

/** Unit tests for {@link BraveYouTubePictureInPictureController}'s state-transition surface. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(shadows = {ShadowLooper.class})
public class BraveYouTubePictureInPictureControllerTest {
    /**
     * The bytecode-injected upstream {@code FullscreenHtmlApiHandlerBase} extends Brave's abstract
     * handler and implements {@code FullscreenManager}. This stub mirrors that generated type in
     * tests.
     */
    private abstract static class FullscreenHandlerStub extends BraveFullscreenHtmlApiHandlerBase
            implements FullscreenManager {}

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BraveActivity mBraveActivity;
    @Mock private WebContents mWebContents;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private FullscreenHandlerStub mBraveFullscreenHandler;
    @Mock private BraveYouTubeScriptInjectorNativeHelper.Natives mNatives;

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
                new BraveYouTubePictureInPictureController(mBraveActivity);

        controller.onSessionRequested(mWebContents);

        assertTrue(controller.isActive());
        assertFalse(controller.isExitingForTesting());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
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
        ShadowKeyguardManager shadowKeyguardManager =
                Shadows.shadowOf(
                        (KeyguardManager)
                                ContextUtils.getApplicationContext()
                                        .getSystemService(Context.KEYGUARD_SERVICE));
        shadowKeyguardManager.setKeyguardLocked(true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        controller.onNewTabDuringPictureInPicture();

        // Session preserved for resume on unlock; no teardown was performed and playback is
        // not paused (the screen-state receiver decides that on unlock).
        assertTrue(controller.isActive());
        assertTrue(controller.isInterruptedByScreenLockForTesting());
        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mWebContents, never()).exitFullscreen();
    }

    @Test
    public void onNewTabDuringPictureInPicture_registryGcedAndTabGone_doesNotRetarget() {
        // Recovery fails cleanly when the tab id no longer resolves to a tab (for example after
        // the YouTube tab was closed during the activity-recreation window). The controller must
        // not fall back to the foreground tab which would silently retarget the session; the
        // operation simply no-ops on the renderer side.
        final int tabId = 42;
        when(mBraveActivity.getTabModelSelector()).thenReturn(mTabModelSelector);
        when(mTabModelSelector.getTabById(tabId)).thenReturn(null);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putInt(BraveYouTubePictureInPictureController.KEY_TAB_ID, tabId);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        controller.onNewTabDuringPictureInPicture();

        verify(mBraveActivity, never()).getCurrentWebContents();
        verify(mWebContents, never()).exitFullscreen();
        // TODO - Uncomment once implemented in core.
        // See https://github.com/brave/brave-core/pull/36087
        // verify(mNatives, never()).exitFullscreen(mWebContents);
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
        when(mBraveActivity.getFullscreenManager()).thenReturn(mBraveFullscreenHandler);
        when(mBraveFullscreenHandler.getPersistentFullscreenMode()).thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(observableWebContents);

        controller.onExitPictureInPictureMode();

        // The JS exit was driven via the JNI helper for the active session.
        // TODO - Uncomment once implemented in core.
        // See https://github.com/brave/brave-core/pull/36087
        // verify(mNatives).exitFullscreen(observableWebContents);

        // Capture the observer the controller installed, then simulate the renderer
        // reporting DOM fullscreen exit.
        ArgumentCaptor<WebContentsObserver> observerCaptor =
                ArgumentCaptor.forClass(WebContentsObserver.class);
        verify((WebContentsObserver.Observable) observableWebContents)
                .addObserver(observerCaptor.capture());
        WebContentsObserver exitObserver = observerCaptor.getValue();
        exitObserver.didToggleFullscreenModeForTab(
                /* enteredFullscreen= */ false, /* willCauseResize= */ false);

        // Fast path fired: cleanup ran and the observer was detached.
        verify(mBraveFullscreenHandler).exitPersistentFullscreenModeForPictureInPicture();
        verify((WebContentsObserver.Observable) observableWebContents).removeObserver(exitObserver);

        // Advance the looper just past the fallback delay so the safety task fires but the
        // delayed dismiss check stays queued (running it would touch MediaSession,
        // which has no JNI seam in this test environment). The fallback must short circuit
        // because the fast path already owned the cleanup.
        ShadowLooper.idleMainLooper(
                BraveYouTubePictureInPictureController.FULLSCREEN_EXIT_FALLBACK_MS,
                TimeUnit.MILLISECONDS);

        verify(mBraveFullscreenHandler, times(1)).exitPersistentFullscreenModeForPictureInPicture();
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
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(observableWebContents);

        controller.onExitPictureInPictureMode();

        // Let the safety timer fire (the renderer signal never arrives in this scenario). Without
        // the guard this delayed task would call getFullscreenManager() on the finishing activity.
        ShadowLooper.idleMainLooper(
                BraveYouTubePictureInPictureController.FULLSCREEN_EXIT_FALLBACK_MS,
                TimeUnit.MILLISECONDS);

        // Guard fired: the activity's fullscreen surface was never touched.
        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mBraveFullscreenHandler, never()).exitPersistentFullscreenModeForPictureInPicture();
    }
}

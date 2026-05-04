/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.KeyguardManager;
import android.content.Context;
import android.os.Bundle;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Shadows;
import org.robolectric.shadows.ShadowKeyguardManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.fullscreen.BraveFullscreenHtmlApiHandlerBase;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.WebContents;

/**
 * Unit tests for {@link BraveYouTubePictureInPictureController}'s pure state-transition surface.
 */
@RunWith(BaseRobolectricTestRunner.class)
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
    @Mock private WebContents mOtherWebContents;
    @Mock private FullscreenManager mFullscreenManager;
    @Mock private FullscreenHandlerStub mBraveFullscreenHandler;
    @Mock private BrowserControlsManager mBrowserControlsManager;
    @Mock private MonotonicObservableSupplier<BrowserControlsManager> mBcmSupplier;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private Tab mTab;
    @Mock private MediaSession mMediaSession;

    @After
    public void tearDown() {
        // The controller writes the active session into a process-wide registry. Reset it so
        // tests don't bleed state into each other.
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(null);
    }

    private Runnable capturePostedUiTask(MockedStatic<PostTask> postTask) {
        ArgumentCaptor<Runnable> taskCaptor = ArgumentCaptor.forClass(Runnable.class);
        postTask.verify(() -> PostTask.postTask(eq(TaskTraits.UI_DEFAULT), taskCaptor.capture()));
        return taskCaptor.getValue();
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
    public void onNewTabDuringPictureInPicture_active_suspendsExitsAndClearsSession() {
        when(mBraveActivity.getFullscreenManager()).thenReturn(mFullscreenManager);
        when(mBraveActivity.getBrowserControlsManagerSupplier()).thenReturn(mBcmSupplier);
        when(mBcmSupplier.get()).thenReturn(mBrowserControlsManager);
        when(mFullscreenManager.getPersistentFullscreenMode()).thenReturn(false);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class);
                MockedStatic<PostTask> postTask = mockStatic(PostTask.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            // YouTube-side player exit is requested via the JS helper.
            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));

            capturePostedUiTask(postTask).run();
        }

        // YouTube playback is paused so audio doesn't continue while the user is on the new tab.
        verify(mMediaSession).suspend();
        // WebContents-level exit is invoked directly on the tracked tab so the renderer drops
        // DOM fullscreen even when FullscreenManager.mWebContentsInFullscreen is stale.
        verify(mWebContents).exitFullscreen();
        verify(mBrowserControlsManager).showAndroidControls(false);
        assertFalse(controller.isActive());
        assertFalse(controller.isInterruptedByScreenLockForTesting());
    }

    @Test
    public void onNewTabDuringPictureInPicture_destroyedBeforePostedCleanup_isNoop() {
        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class);
                MockedStatic<PostTask> postTask = mockStatic(PostTask.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));
            Runnable postedCleanup = capturePostedUiTask(postTask);
            controller.onDestroy();

            postedCleanup.run();
        }

        verify(mBraveActivity, never()).getFullscreenManager();
        verify(mBraveActivity, never()).getTabModelSelector();
        verify(mBraveActivity, never()).getBrowserControlsManagerSupplier();
        assertFalse(controller.isActive());
    }

    @Test
    public void onNewTabDuringPictureInPicture_restoredSession_usesRegisteredWebContents() {
        when(mBraveActivity.getFullscreenManager()).thenReturn(mFullscreenManager);
        when(mBraveActivity.getBrowserControlsManagerSupplier()).thenReturn(mBcmSupplier);
        when(mBcmSupplier.get()).thenReturn(mBrowserControlsManager);
        when(mFullscreenManager.getPersistentFullscreenMode()).thenReturn(false);
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(mWebContents);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class);
                MockedStatic<PostTask> postTask = mockStatic(PostTask.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));
            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mOtherWebContents),
                    never());

            capturePostedUiTask(postTask).run();
        }

        verify(mBraveActivity, never()).getCurrentWebContents();
        verify(mWebContents).exitFullscreen();
        verify(mOtherWebContents, never()).exitFullscreen();
        verify(mBrowserControlsManager).showAndroidControls(false);
        assertFalse(controller.isActive());
    }

    @Test
    public void onNewTabDuringPictureInPicture_registryGcedRecoversByTabId() {
        // After activity recreation the registry's WeakReference can be cleared by GC. The
        // saved-instance bundle carries the owning tab id, so the controller must recover the
        // WebContents via TabModelSelector instead of falling back to the foreground tab which
        // could be a different tab if the user switched during recreation.
        final int tabId = 42;
        when(mBraveActivity.getFullscreenManager()).thenReturn(mFullscreenManager);
        when(mBraveActivity.getBrowserControlsManagerSupplier()).thenReturn(mBcmSupplier);
        when(mBcmSupplier.get()).thenReturn(mBrowserControlsManager);
        when(mFullscreenManager.getPersistentFullscreenMode()).thenReturn(false);
        when(mBraveActivity.getTabModelSelector()).thenReturn(mTabModelSelector);
        when(mTabModelSelector.getTabById(tabId)).thenReturn(mTab);
        when(mTab.getWebContents()).thenReturn(mWebContents);

        Bundle savedState = new Bundle();
        savedState.putBoolean(BraveYouTubePictureInPictureController.KEY_ACTIVE, true);
        savedState.putInt(BraveYouTubePictureInPictureController.KEY_TAB_ID, tabId);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onPostCreate(savedState);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class);
                MockedStatic<PostTask> postTask = mockStatic(PostTask.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));
            capturePostedUiTask(postTask).run();
        }

        // Recovered the right WebContents and never queried the foreground tab.
        verify(mTabModelSelector).getTabById(tabId);
        verify(mBraveActivity, never()).getCurrentWebContents();
        verify(mWebContents).exitFullscreen();
        assertFalse(controller.isActive());
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

        try (MockedStatic<PostTask> postTask = mockStatic(PostTask.class)) {
            controller.onNewTabDuringPictureInPicture();
        }

        verify(mBraveActivity, never()).getCurrentWebContents();
        verify(mWebContents, never()).exitFullscreen();
    }

    @Test
    public void onNewTabDuringPictureInPicture_persistentFullscreen_exitsBraveUi() {
        when(mBraveActivity.getFullscreenManager()).thenReturn(mBraveFullscreenHandler);
        when(mBraveActivity.getTabModelSelector()).thenReturn(mTabModelSelector);
        when(mTabModelSelector.getCurrentTab()).thenReturn(mTab);
        when(mBraveActivity.getBrowserControlsManagerSupplier()).thenReturn(mBcmSupplier);
        when(mBcmSupplier.get()).thenReturn(mBrowserControlsManager);
        when(mBraveFullscreenHandler.getPersistentFullscreenMode()).thenReturn(true);

        BraveYouTubePictureInPictureController controller =
                new BraveYouTubePictureInPictureController(mBraveActivity);
        controller.onSessionRequested(mWebContents);

        try (MockedStatic<BraveYouTubeScriptInjectorNativeHelper> nativeHelper =
                        mockStatic(BraveYouTubeScriptInjectorNativeHelper.class);
                MockedStatic<MediaSession> mediaSessionStatic = mockStatic(MediaSession.class);
                MockedStatic<PostTask> postTask = mockStatic(PostTask.class)) {
            mediaSessionStatic
                    .when(() -> MediaSession.fromWebContents(mWebContents))
                    .thenReturn(mMediaSession);

            controller.onNewTabDuringPictureInPicture();

            nativeHelper.verify(
                    () -> BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(mWebContents));

            capturePostedUiTask(postTask).run();
        }

        // The posted cleanup uses the canonical fullscreen exit path after the tab-change observer
        // settles, then forces browser controls visible for tablet toolbar positioning.
        verify(mBraveFullscreenHandler).onExitFullscreen(mTab);
        verify(mBraveFullscreenHandler, never()).exitPersistentFullscreenModeForPictureInPicture();
        verify(mBrowserControlsManager).showAndroidControls(false);
        verify(mMediaSession).suspend();
        verify(mWebContents).exitFullscreen();
        assertFalse(controller.isActive());
    }
}

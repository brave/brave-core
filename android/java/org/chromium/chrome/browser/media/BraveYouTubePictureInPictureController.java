/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import android.app.KeyguardManager;
import android.app.PictureInPictureParams;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.PowerManager;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.fullscreen.BraveFullscreenHtmlApiHandlerBase;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.WebContents;

/**
 * Owns the lifecycle of a Brave-managed YouTube Picture-in-Picture session.
 *
 * <p>A session begins when either the manual YouTube PiP button or the upstream fullscreen-video
 * PiP path requests it via {@link #onSessionRequested}; it ends either when PiP is dismissed
 * normally, when the screen-lock interrupt sequence resolves, or when the entry attempt fails.
 */
@NullMarked
public class BraveYouTubePictureInPictureController {
    private static final String TAG = "BraveYTPiPController";

    @VisibleForTesting
    static final String KEY_ACTIVE = "org.chromium.chrome.browser.media.YT_PIP_ACTIVE";

    @VisibleForTesting
    static final String KEY_INTERRUPTED_BY_SCREEN_LOCK =
            "org.chromium.chrome.browser.media.YT_PIP_INTERRUPTED_BY_SCREEN_LOCK";

    @VisibleForTesting
    static final String KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY =
            "org.chromium.chrome.browser.media.YT_PIP_RESUME_MEDIA_ON_ENTRY";

    /**
     * Time given to YouTube's DOM to drop out of fullscreen after we ask it to. Picked empirically:
     * shorter values cause the browser-chrome restore to race with the JS exit; longer values are
     * visible to the user as a delay before chrome reappears.
     */
    @VisibleForTesting static final long FULLSCREEN_EXIT_DELAY_MS = 300L;

    /**
     * Time given to the user/system to confirm dismissal of the PiP window before we suspend the
     * media session. Long enough to cover an Android animation back to the activity, short enough
     * that a true dismiss feels responsive.
     */
    @VisibleForTesting static final long DISMISS_CHECK_DELAY_MS = 1000L;

    private final BraveActivity mActivity;

    private @Nullable WebContents mWebContents;
    private boolean mActive;
    private boolean mExiting;
    private boolean mInterruptedByScreenLock;
    private int mSessionId;
    private @Nullable BroadcastReceiver mScreenStateReceiver;

    /**
     * True iff a PiP entry should resume the media session once the system reports the activity is
     * in PiP mode. Set by {@link #onSessionRequested} and consumed by {@link
     * #onEnterPictureInPictureMode}.
     */
    private boolean mResumeMediaSessionOnPipEntry;

    public BraveYouTubePictureInPictureController(BraveActivity activity) {
        mActivity = activity;
    }

    /** Restore persisted state on activity creation. */
    public void onPostCreate(@Nullable Bundle savedInstanceState) {
        if (savedInstanceState == null) {
            return;
        }
        mActive = savedInstanceState.getBoolean(KEY_ACTIVE, false);
        mInterruptedByScreenLock =
                savedInstanceState.getBoolean(KEY_INTERRUPTED_BY_SCREEN_LOCK, false);
        mResumeMediaSessionOnPipEntry =
                savedInstanceState.getBoolean(KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY, false);
        if (mActive && mInterruptedByScreenLock) {
            registerScreenStateReceiver();
        }
    }

    /** Persist state ahead of activity recreation. */
    public void onSaveInstanceState(Bundle outState) {
        outState.putBoolean(KEY_ACTIVE, mActive);
        outState.putBoolean(KEY_INTERRUPTED_BY_SCREEN_LOCK, mInterruptedByScreenLock);
        outState.putBoolean(KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY, mResumeMediaSessionOnPipEntry);
    }

    /** Hook from {@code Activity#onResume}. */
    public void onResume() {
        maybeResumeAfterScreenLock();
    }

    /** Hook from {@code Activity#onDestroy}. */
    public void onDestroy() {
        unregisterScreenStateReceiver();
        // On a configuration change the recreated activity restores the session from the
        // saved-instance bundle, so leave the registry intact for it to pick up.
        if (!mActivity.isChangingConfigurations()) {
            BraveMediaSessionHelper.clearYouTubePictureInPictureWebContents(mWebContents);
        }
        // Reset session state so any delayed callbacks queued by handleSessionExited() return
        // early via isExitingForSession() instead of touching a destroyed activity. State has
        // already been written to the saved-instance bundle, so the next activity restores from
        // the bundle rather than from these fields.
        resetSessionState();
    }

    /**
     * Hook from {@code onPictureInPictureModeChanged(true)}. Resets any prior interrupt state,
     * adopts a system-initiated PiP if this is a YouTube tab, and resumes the media session if a
     * pending request asked us to.
     *
     * @return true if the caller should run the PiP-bounds-update workaround. (Returned for
     *     symmetry with the previous BraveActivity behavior.)
     */
    public boolean onEnterPictureInPictureMode() {
        mExiting = false;
        mInterruptedByScreenLock = false;
        maybeAdoptSession();

        if (!mResumeMediaSessionOnPipEntry) {
            return false;
        }
        mResumeMediaSessionOnPipEntry = false;

        // The caller (BraveActivity.onPictureInPictureModeChanged) currently looks at the
        // activity's current WebContents to find the media session. We mirror that to keep
        // behavior identical: if the tab changed between request and entry, the activity-side
        // resume still targets the now-foreground tab. (Worst case: a benign no-op on a tab
        // without a media session.)
        final WebContents wc = mActivity.getCurrentWebContents();
        MediaSession mediaSession = wc != null ? MediaSession.fromWebContents(wc) : null;
        if (mediaSession != null) {
            mediaSession.resume();
        }
        return true;
    }

    /** Hook from {@code onPictureInPictureModeChanged(false)}. */
    public void onExitPictureInPictureMode() {
        if (!mActive) {
            return;
        }
        if (isScreenOffOrLocked(mActivity)) {
            markInterruptedByScreenLock();
            return;
        }
        handleSessionExited();
    }

    /**
     * Marks that a Brave-managed YouTube PiP session is starting against {@code webContents}. The
     * caller is expected to invoke {@code Activity#enterPictureInPictureMode} immediately after.
     */
    public void onSessionRequested(final WebContents webContents) {
        mResumeMediaSessionOnPipEntry = true;
        mWebContents = webContents;
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(webContents);
        mActive = true;
        mExiting = false;
        mInterruptedByScreenLock = false;
        mSessionId++;
    }

    /** The corresponding PiP entry attempt failed. Abandon the session. */
    public void onSessionEnterFailed() {
        mResumeMediaSessionOnPipEntry = false;
        clearSession(mSessionId, mWebContents);
    }

    public boolean isActive() {
        return mActive;
    }

    /**
     * Signaled by the upstream PiP exit observers when YouTube reports leaving fullscreen while the
     * device looks asleep. Treat as a transient interruption.
     */
    public void onFullscreenInterrupted() {
        if (mActive) {
            markInterruptedByScreenLock();
        }
    }

    /**
     * Called when a new tab arrives during an active YouTube PiP session (typically via a launcher
     * shortcut). Exit persistent fullscreen UI synchronously and release the YouTube WebContents
     * from fullscreen state so it is in a normal layout when the user switches back.
     */
    public void onNewTabDuringPictureInPicture() {
        if (!mActive) {
            return;
        }

        // Mirror onExitPictureInPictureMode: defer teardown while the device is locked and let
        // the screen-state receiver pick the session back up on unlock.
        if (isScreenOffOrLocked(mActivity)) {
            markInterruptedByScreenLock();
            return;
        }

        final WebContents webContents = getOrFindWebContents();
        if (webContents != null && !webContents.isDestroyed()) {
            // Pause YouTube playback so audio does not continue while the user is on the new
            // tab. Mirrors maybeSuspendDismissed() but without the visibility check, since a
            // new-tab event is an explicit "user moved on" signal.
            final MediaSession mediaSession = MediaSession.fromWebContents(webContents);
            if (mediaSession != null) {
                mediaSession.suspend();
            }
            // Nudge YouTube's player out of its own fullscreen state, since YouTube tracks
            // fullscreen independently of the DOM.
            BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(webContents);
            // Tell the YouTube WebContents to drop fullscreen directly. We do not rely on
            // FullscreenManager.exitPersistentFullscreenMode() to forward this: by the
            // time the tab-change observer has fired and we get here, the manager's internal
            // mWebContentsInFullscreen pointer can already be stale, and the WebContents-level
            // exit it would otherwise emit silently no-ops. Calling exitFullscreen() directly on
            // the tracked YouTube WebContents ensures the renderer leaves fullscreen so the tab
            // is in a normal layout when the user switches back.
            webContents.exitFullscreen();
        }

        // Defer the layout-side exit. We are called from upstream
        // DismissActivityOnTabChangeObserver.onResult, which still runs registerTabEventObserver
        // and updateAutoPictureInPictureStatusIfNeeded after we return. Posting to UI_DEFAULT
        // lets the rest of the tab-change chain settle first.
        final int sessionId = mSessionId;
        final WebContents capturedWebContents = mWebContents;
        PostTask.postTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    if (!mActive
                            || sessionId != mSessionId
                            || capturedWebContents != mWebContents
                            || mActivity.isActivityFinishingOrDestroyed()) {
                        return;
                    }
                    final FullscreenManager fullscreenManager = mActivity.getFullscreenManager();
                    final Tab activityTab = mActivity.getActivityTab();
                    if (fullscreenManager.getPersistentFullscreenMode() && activityTab != null) {
                        // Canonical exit path: clears persistent-fullscreen layout state and
                        // fans out to FullscreenManager.Observer (InfoBarContainer,
                        // ContextualSearchManager, navbar coloring, etc.).
                        fullscreenManager.onExitFullscreen(activityTab);
                    }
                    final BrowserControlsManager bcm = mActivity.getBrowserControlsManager();
                    if (bcm != null) {
                        // Force the browser-controls offset back to 0. On tablets the toolbar can
                        // stay translated off-screen after the canonical exit and
                        // BrowserControlsManager.onConstraintsChanged doesn't always recover it.
                        // Writing the offsets directly is the most reliable way to bring it back.
                        bcm.showAndroidControls(false);
                    }
                    clearSession(sessionId, capturedWebContents);
                });
    }

    /**
     * Returns true if the device screen is off or the keyguard is locked. Shared by the controller
     * and {@link BraveFullscreenVideoPictureInPictureController}.
     */
    public static boolean isScreenOffOrLocked(Context context) {
        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (powerManager != null && !powerManager.isInteractive()) {
            return true;
        }

        KeyguardManager keyguardManager =
                (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        return keyguardManager != null && keyguardManager.isKeyguardLocked();
    }

    private void maybeAdoptSession() {
        if (mActive) {
            return;
        }

        final WebContents currentWebContents = mActivity.getCurrentWebContents();
        if (currentWebContents == null
                || currentWebContents.isDestroyed()
                || !BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        currentWebContents)) {
            return;
        }

        onSessionRequested(currentWebContents);
    }

    private void handleSessionExited() {
        if (mExiting) {
            return;
        }

        final WebContents webContents = getOrFindWebContents();
        if (webContents == null || webContents.isDestroyed()) {
            clearSession(mSessionId, mWebContents);
            return;
        }

        final int sessionId = mSessionId;
        mExiting = true;
        BraveYouTubeScriptInjectorNativeHelper.exitFullscreen(webContents);

        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> maybeRestoreFullscreenUi(sessionId, webContents),
                FULLSCREEN_EXIT_DELAY_MS);
        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> maybeSuspendDismissed(sessionId, webContents),
                DISMISS_CHECK_DELAY_MS);
    }

    @Nullable
    private WebContents getOrFindWebContents() {
        if (mWebContents != null) {
            // The session is bound to a specific WebContents. If it has since been destroyed the
            // YouTube tab is gone for good, so refuse to rebind to whatever foreground tab is
            // current — that would silently retarget exit logic at the wrong WebContents.
            return mWebContents.isDestroyed() ? null : mWebContents;
        }

        // mWebContents was never set on this controller instance (typically after activity
        // recreation, when mActive is restored from the saved-instance bundle but the WebContents
        // reference cannot be persisted). Prefer the process-wide registry retained across the
        // configuration change, since the foreground tab may already be the newly opened tab.
        final WebContents pictureInPictureWebContents =
                BraveMediaSessionHelper.getYouTubePictureInPictureWebContents();
        if (pictureInPictureWebContents != null) {
            mWebContents = pictureInPictureWebContents;
            return pictureInPictureWebContents;
        }

        // If the registry is empty, fall back to the foreground tab for restore paths where the
        // current tab is still the YouTube PiP tab.
        final WebContents currentWebContents = mActivity.getCurrentWebContents();
        if (currentWebContents != null
                && !currentWebContents.isDestroyed()
                && BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        currentWebContents)) {
            mWebContents = currentWebContents;
            BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(currentWebContents);
            return currentWebContents;
        }
        return null;
    }

    private void maybeRestoreFullscreenUi(final int sessionId, final WebContents webContents) {
        if (!isExitingForSession(sessionId, webContents) || mActivity.isInPictureInPictureMode()) {
            return;
        }

        if (isScreenOffOrLocked(mActivity)) {
            markInterruptedByScreenLock();
            return;
        }

        final FullscreenManager fullscreenManager = mActivity.getFullscreenManager();
        if (fullscreenManager.getPersistentFullscreenMode()
                && fullscreenManager instanceof final BraveFullscreenHtmlApiHandlerBase brave) {
            brave.exitPersistentFullscreenModeForPictureInPicture();
        }
    }

    private void maybeSuspendDismissed(final int sessionId, final WebContents webContents) {
        if (!isExitingForSession(sessionId, webContents)) {
            return;
        }

        if (isScreenOffOrLocked(mActivity)) {
            markInterruptedByScreenLock();
            return;
        }

        if (!isActivityVisibleAfterPictureInPicture()) {
            final MediaSession mediaSession = MediaSession.fromWebContents(webContents);
            if (mediaSession != null) {
                mediaSession.suspend();
            }
        }
        clearSession(sessionId, webContents);
    }

    private boolean isActivityVisibleAfterPictureInPicture() {
        return ApplicationStatus.getStateForActivity(mActivity) == ActivityState.RESUMED
                && mActivity.hasWindowFocus();
    }

    private void maybeResumeAfterScreenLock() {
        if (!mActive || !mInterruptedByScreenLock || isScreenOffOrLocked(mActivity)) {
            return;
        }

        final WebContents webContents = getOrFindWebContents();
        if (webContents == null || webContents.isDestroyed()) {
            clearSession(mSessionId, mWebContents);
            return;
        }

        // Conditions under which we cannot re-enter PiP. Bail out *before* mutating state so the
        // session stays consistent and the receiver is still wired up for a later wake-up.
        if (mActivity.isInPictureInPictureMode()
                || mActivity.isChangingConfigurations()
                || mActivity.isFinishing()
                || !BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        webContents)) {
            return;
        }

        mInterruptedByScreenLock = false;
        unregisterScreenStateReceiver();

        // Defer resuming the media session until Android confirms PiP entry. Mirrors the path
        // taken by an initial PiP request: onEnterPictureInPictureMode() will consume this flag
        // from onPictureInPictureModeChanged(true). If PiP entry fails below, we clear the
        // session without ever resuming, so the user does not get foreground audio after
        // unlocking.
        mResumeMediaSessionOnPipEntry = true;

        try {
            if (!mActivity.enterPictureInPictureMode(
                    new PictureInPictureParams.Builder().build())) {
                mResumeMediaSessionOnPipEntry = false;
                clearSession(mSessionId, webContents);
            }
        } catch (IllegalStateException | IllegalArgumentException e) {
            Log.e(TAG, "Error restoring YouTube picture-in-picture mode.", e);
            mResumeMediaSessionOnPipEntry = false;
            clearSession(mSessionId, webContents);
        }
    }

    private void markInterruptedByScreenLock() {
        mInterruptedByScreenLock = true;
        registerScreenStateReceiver();
    }

    private void registerScreenStateReceiver() {
        if (mScreenStateReceiver != null) {
            return;
        }

        mScreenStateReceiver =
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        final String action = intent.getAction();
                        if (!Intent.ACTION_SCREEN_ON.equals(action)
                                && !Intent.ACTION_USER_PRESENT.equals(action)) {
                            return;
                        }
                        maybeResumeAfterScreenLock();
                    }
                };
        final IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_USER_PRESENT);
        ContextUtils.registerProtectedBroadcastReceiver(mActivity, mScreenStateReceiver, filter);
    }

    private void unregisterScreenStateReceiver() {
        if (mScreenStateReceiver == null) {
            return;
        }

        try {
            mActivity.unregisterReceiver(mScreenStateReceiver);
        } catch (IllegalArgumentException ignored) {
            // Screen-state receiver was already unregistered.
        }
        mScreenStateReceiver = null;
    }

    /**
     * Returns true iff the (sessionId, webContents) pair refers to the currently *exiting* YT-PiP
     * session. The {@code mExiting} guard is intentional: callers of this helper are scheduled
     * during exit teardown and must no-op if the session was renewed in the meantime.
     *
     * @noinspection BooleanMethodIsAlwaysInverted
     */
    private boolean isExitingForSession(final int sessionId, final WebContents webContents) {
        return mActive
                && mExiting
                && sessionId == mSessionId
                && webContents == mWebContents
                && !webContents.isDestroyed();
    }

    private void clearSession(final int sessionId, @Nullable final WebContents webContents) {
        if (sessionId != mSessionId || webContents != mWebContents) {
            return;
        }

        unregisterScreenStateReceiver();
        BraveMediaSessionHelper.clearYouTubePictureInPictureWebContents(webContents);
        resetSessionState();
    }

    /**
     * Resets the in-memory session fields. Shared by {@link #clearSession} and {@link #onDestroy};
     * receiver unregistration and registry clearing stay at the call sites because their gating
     * differs (session-id match vs. configuration-change).
     */
    private void resetSessionState() {
        mActive = false;
        mExiting = false;
        mInterruptedByScreenLock = false;
        mResumeMediaSessionOnPipEntry = false;
        mWebContents = null;
    }

    @VisibleForTesting
    public boolean isExitingForTesting() {
        return mExiting;
    }

    @VisibleForTesting
    public boolean isInterruptedByScreenLockForTesting() {
        return mInterruptedByScreenLock;
    }

    @VisibleForTesting
    public boolean hasScreenStateReceiverForTesting() {
        return mScreenStateReceiver != null;
    }
}

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
        if (!mActivity.isChangingConfigurations()) {
            BraveMediaSessionHelper.clearYouTubePictureInPictureWebContents(mWebContents);
        }
        // Reset session state so any delayed callbacks queued by handleSessionExited() return
        // early via isExitingForSession() instead of touching a destroyed activity. State has
        // already been written to the saved instance bundle, so the next activity restores from
        // the bundle rather than from these fields.
        mActive = false;
        mExiting = false;
        mInterruptedByScreenLock = false;
        mResumeMediaSessionOnPipEntry = false;
        mWebContents = null;
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
        if (mWebContents != null && !mWebContents.isDestroyed()) {
            return mWebContents;
        }
        WebContents currentWebContents = mActivity.getCurrentWebContents();
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
        assert fullscreenManager instanceof BraveFullscreenHtmlApiHandlerBase;
        if (fullscreenManager.getPersistentFullscreenMode()) {
            BraveFullscreenHtmlApiHandlerBase braveFullscreen =
                    (BraveFullscreenHtmlApiHandlerBase) fullscreenManager;
            braveFullscreen.exitPersistentFullscreenModeForPictureInPicture();
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

        mActive = false;
        mExiting = false;
        mInterruptedByScreenLock = false;
        BraveMediaSessionHelper.clearYouTubePictureInPictureWebContents(webContents);
        unregisterScreenStateReceiver();
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

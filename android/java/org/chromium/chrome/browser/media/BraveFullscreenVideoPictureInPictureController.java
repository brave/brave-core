/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import android.app.Activity;

import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.content_public.browser.WebContents;

public class BraveFullscreenVideoPictureInPictureController {
    /**
     * This variable will be used instead of {@link FullscreenVideoPictureInPictureController}'s
     * variable, that will be deleted in bytecode.
     */
    protected boolean mDismissPending;

    /**
     * Tracks the Brave activity that owns the YouTube PiP session being attempted in
     * attemptPictureInPicture, so we can notify it if entry fails.
     */
    @Nullable private BraveActivity mPendingBraveActivityForPiP;

    /**
     * Called by upstream's attemptPictureInPicture before entering PiP. Resolves the Brave
     * activity that should manage the upcoming YouTube PiP session (or null if this is not a
     * Brave-managed YouTube PiP attempt) and notifies it that a session is being requested.
     */
    protected void onYouTubePictureInPictureAttempt(Activity activity, WebContents webContents) {
        mPendingBraveActivityForPiP =
                resolveBraveActivityForYouTubePictureInPicture(activity, webContents);
        if (mPendingBraveActivityForPiP != null) {
            mPendingBraveActivityForPiP.onYouTubePictureInPictureRequested(webContents);
        }
    }

    /**
     * Called by upstream when {@code enterPictureInPictureMode} returns false or throws, so the
     * Brave activity can roll back any YouTube PiP session state set up in
     * {@link #onYouTubePictureInPictureAttempt}.
     */
    protected void onYouTubePictureInPictureEnterFailed() {
        if (mPendingBraveActivityForPiP != null) {
            mPendingBraveActivityForPiP.onYouTubePictureInPictureEnterFailed();
            mPendingBraveActivityForPiP = null;
        }
    }

    private @Nullable BraveActivity resolveBraveActivityForYouTubePictureInPicture(
            Activity activity, WebContents webContents) {
        if (!(activity instanceof BraveActivity braveActivity)
                || !BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        webContents)) {
            return null;
        }
        return braveActivity;
    }

    protected boolean maybeHandleDismissActivityForYouTubePictureInPicture(
            Activity activity,
            boolean isStart,
            boolean isResume,
            boolean isLeftFullscreen,
            boolean isWebContentsLeftFullscreen,
            boolean isNewTab) {
        if (shouldDeferDismissForYouTubePictureInPicture(activity, isStart, isResume)
                || shouldKeepPictureInPictureAlive(
                        activity, isLeftFullscreen, isWebContentsLeftFullscreen)
                || shouldStayInForegroundForNewTab(activity, isNewTab)) {
            mDismissPending = false;
            return true;
        }
        return false;
    }

    private boolean shouldDeferDismissForYouTubePictureInPicture(
            Activity activity, boolean isStart, boolean isResume) {
        if (!isStart && !isResume) {
            return false;
        }

        return activity instanceof final BraveActivity braveActivity
                && braveActivity.isYouTubePictureInPictureActive();
    }

    private boolean shouldKeepPictureInPictureAlive(
            Activity activity, boolean isLeftFullscreen, boolean isWebContentsLeftFullscreen) {
        if (!isLeftFullscreen && !isWebContentsLeftFullscreen) {
            return false;
        }

        // Only act when an active YouTube PiP session managed by Brave is in flight. Without
        // this guard, we would swallow upstream's dismiss for unrelated PiP sessions whenever
        // the screen happens to be locked.
        if (!(activity instanceof final BraveActivity braveActivity)
                || !braveActivity.isYouTubePictureInPictureActive()) {
            return false;
        }

        if (BraveYouTubePictureInPictureController.isScreenOffOrLocked()) {
            braveActivity.onYouTubePictureInPictureFullscreenInterrupted();
            return true;
        }

        // Transient upstream signals (e.g. effective-fullscreen toggling while the device is
        // waking) can report that YouTube left fullscreen while the task is still pinned. Keep
        // the Brave-managed YouTube PiP window alive; DOM + persistent fullscreen state is
        // preserved across the transition by BraveFullscreenHtmlApiHandlerBase.
        return activity.isInPictureInPictureMode();
    }

    private boolean shouldStayInForegroundForNewTab(Activity activity, boolean isNewTab) {
        if (!isNewTab) {
            return false;
        }

        // When a new tab arrives during a Brave-managed YouTube PiP session (commonly via a
        // launcher shortcut), upstream calls activity.moveTaskToBack(true) and sends the
        // activity behind the home screen. Returning true here skips the upstream dismiss path
        // so the new tab becomes visible and PiP exits naturally through
        // onPictureInPictureModeChanged when Android brings the activity to the foreground for
        // the new intent. Also drop Brave's persistent fullscreen UI synchronously so the new
        // tab is not rendered on a residual fullscreen layout.
        if (!(activity instanceof final BraveActivity braveActivity)
                || !braveActivity.isYouTubePictureInPictureActive()
                || !activity.isInPictureInPictureMode()) {
            return false;
        }

        braveActivity.onYouTubePictureInPictureNewTab();
        return true;
    }
}

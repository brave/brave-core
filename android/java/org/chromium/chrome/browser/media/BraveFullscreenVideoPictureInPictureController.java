/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import android.app.Activity;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.app.BraveActivity;

public class BraveFullscreenVideoPictureInPictureController {
    // Mirrors the int values of FullscreenVideoPictureInPictureController.MetricsEndReason in
    // upstream Chromium. Update if upstream renumbers — checked manually since the upstream enum
    // is package-private and not directly importable.
    // Source: chrome/android/java/src/org/chromium/chrome/browser/media/
    //         FullscreenVideoPictureInPictureController.java#MetricsEndReason
    private static final int METRICS_END_REASON_RESUME = 0;
    private static final int METRICS_END_REASON_LEFT_FULLSCREEN = 6;
    private static final int METRICS_END_REASON_WEB_CONTENTS_LEFT_FULLSCREEN = 7;
    private static final int METRICS_END_REASON_START = 8;

    /**
     * This variable will be used instead of {@link FullscreenVideoPictureInPictureController}'s
     * variable, that will be deleted in bytecode.
     */
    protected boolean mDismissPending;

    void dismissActivityIfNeeded(Activity activity, /*MetricsEndReason*/ int reason) {
        if (shouldDeferDismissForYouTubePictureInPicture(activity, reason)
                || shouldKeepPictureInPictureAlive(activity, reason)) {
            mDismissPending = false;
            return;
        }
        BraveReflectionUtil.invokeMethod(
                FullscreenVideoPictureInPictureController.class,
                this,
                "dismissActivityIfNeeded",
                Activity.class,
                activity,
                int.class,
                reason);
    }

    private boolean shouldDeferDismissForYouTubePictureInPicture(
            Activity activity, /*MetricsEndReason*/ int reason) {
        if (reason != METRICS_END_REASON_START && reason != METRICS_END_REASON_RESUME) {
            return false;
        }

        return activity instanceof final BraveActivity braveActivity
                && braveActivity.isYouTubePictureInPictureActive();
    }

    private boolean shouldKeepPictureInPictureAlive(
            Activity activity, /*MetricsEndReason*/ int reason) {
        if (reason != METRICS_END_REASON_LEFT_FULLSCREEN
                && reason != METRICS_END_REASON_WEB_CONTENTS_LEFT_FULLSCREEN) {
            return false;
        }

        if (BraveYouTubePictureInPictureController.isScreenOffOrLocked(activity)) {
            if (activity instanceof final BraveActivity braveActivity) {
                braveActivity.onYouTubePictureInPictureFullscreenInterrupted();
            }
            return true;
        }

        // Transient upstream signals (e.g. effective-fullscreen toggling while the device is
        // waking) can report that YouTube left fullscreen while the task is still pinned. Keep
        // the Brave-managed YouTube PiP window alive; DOM + persistent fullscreen state is
        // preserved across the transition by BraveFullscreenHtmlApiHandlerBase.
        return activity.isInPictureInPictureMode()
                && activity instanceof final BraveActivity braveActivity
                && braveActivity.isYouTubePictureInPictureActive();
    }
}

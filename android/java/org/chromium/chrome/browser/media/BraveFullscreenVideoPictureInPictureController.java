/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.Context;
import android.os.PowerManager;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.app.BraveActivity;

public class BraveFullscreenVideoPictureInPictureController {
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
        if (reason == METRICS_END_REASON_START
                || reason == METRICS_END_REASON_RESUME
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

    private boolean shouldKeepPictureInPictureAlive(
            Activity activity, /*MetricsEndReason*/ int reason) {
        if (reason != METRICS_END_REASON_LEFT_FULLSCREEN
                && reason != METRICS_END_REASON_WEB_CONTENTS_LEFT_FULLSCREEN) {
            return false;
        }

        if (isScreenOffOrLocked(activity)) {
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

    private boolean isScreenOffOrLocked(Activity activity) {
        PowerManager powerManager = (PowerManager) activity.getSystemService(Context.POWER_SERVICE);
        if (powerManager != null && !powerManager.isInteractive()) {
            return true;
        }

        KeyguardManager keyguardManager =
                (KeyguardManager) activity.getSystemService(Context.KEYGUARD_SERVICE);
        return keyguardManager != null && keyguardManager.isKeyguardLocked();
    }
}

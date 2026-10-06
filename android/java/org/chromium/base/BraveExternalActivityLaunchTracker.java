/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import android.os.SystemClock;

import org.chromium.build.annotations.NullMarked;

/**
 * Records when Brave last launched an external activity expected to hand control back shortly (a
 * file picker, a share sheet, an app opened from a link) — a plain timestamp, with no opinion on
 * what a caller does with it.
 *
 * <p>Lives in {@code org.chromium.base} (compiled into the real {@code //base:base_java}, not
 * {@code chrome_java}) specifically so the low-level call sites that need to call {@link
 * #notifyLaunchingExternalActivity()} (in {@code ui/android} and {@code components/*}, which cannot
 * depend on anything in {@code chrome/*}) can see it. {@link
 * org.chromium.chrome.browser.privacy.BraveBrowserLockManager} is the sole consumer of {@link
 * #consumeElapsedMsSinceLastLaunch()}, and owns the actual decision of what counts as "shortly
 * after".
 */
@NullMarked
public final class BraveExternalActivityLaunchTracker {
    private static long sLastLaunchTimeMs = -1;

    private BraveExternalActivityLaunchTracker() {}

    /**
     * Call immediately before launching an external activity that is expected to return control to
     * the caller shortly after (e.g. {@code startActivityForResult}, a share intent, or a link that
     * opens another app).
     */
    public static void notifyLaunchingExternalActivity() {
        sLastLaunchTimeMs = SystemClock.elapsedRealtime();
    }

    /**
     * Returns the elapsed time since the most recent {@link #notifyLaunchingExternalActivity()}
     * call, in milliseconds, or -1 if none has been recorded (or it was already consumed). Consumes
     * the record, so a later call returns -1 until the next launch.
     */
    public static long consumeElapsedMsSinceLastLaunch() {
        if (sLastLaunchTimeMs < 0) return -1;
        long elapsed = SystemClock.elapsedRealtime() - sLastLaunchTimeMs;
        sLastLaunchTimeMs = -1;
        return elapsed;
    }

    public static void resetForTesting() {
        sLastLaunchTimeMs = -1;
    }
}

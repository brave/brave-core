/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.annotation.SuppressLint;

import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.util.Timer;
import java.util.TimerTask;

public class UsageMonitor {
    private static final Object sLock = new Object();
    private static final long REPORT_INTERVAL_MS = 15000;

    private MiscAndroidMetrics mMiscAndroidMetrics;
    private Timer mTimer;
    private static UsageMonitor sInstance;

    public static UsageMonitor getInstance(MiscAndroidMetrics miscAndroidMetrics) {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new UsageMonitor();
            }
            sInstance.setMiscAndroidMetrics(miscAndroidMetrics);
        }
        return sInstance;
    }

    private UsageMonitor() {}

    private void setMiscAndroidMetrics(MiscAndroidMetrics miscAndroidMetrics) {
        mMiscAndroidMetrics = miscAndroidMetrics;
    }

    // Warning: Use of scheduleAtFixedRate is strongly discouraged because it can lead to unexpected
    // behavior when Android processes become cached (tasks may unexpectedly execute hundreds or
    // thousands of times in quick succession when a process changes from cached to uncached);
    // prefer using schedule [DiscouragedApi].
    @SuppressLint("DiscouragedApi")
    public void start() {
        if (mTimer != null) {
            mTimer.cancel();
        }
        mTimer = new Timer();
        mTimer.scheduleAtFixedRate(
                new TimerTask() {
                    @Override
                    public void run() {
                        try {
                            TimeDelta duration = new TimeDelta();
                            duration.microseconds = REPORT_INTERVAL_MS * 1000;
                            mMiscAndroidMetrics.recordBrowserUsageDuration(duration);
                        } catch (Exception exc) {
                            assert false : "UsageMonitor exception" + exc.getMessage();
                            // Ignore any kind of exception as it's fine if some pings fail
                        }
                    }
                },
                REPORT_INTERVAL_MS,
                REPORT_INTERVAL_MS);
    }

    public void stop() {
        if (mTimer == null) {
            return;
        }
        mTimer.cancel();
        mTimer = null;
    }
}

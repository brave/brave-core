/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.util.Timer;
import java.util.TimerTask;

public class UsageMonitor {
    private static final long REPORT_INTERVAL_MS = 15000;

    private MiscAndroidMetrics mMiscAndroidMetrics;
    private Timer mTimer;

    public UsageMonitor(MiscAndroidMetrics miscAndroidMetrics) {
        mMiscAndroidMetrics = miscAndroidMetrics;
    }

    public void start() {
        if (mTimer != null) {
            mTimer.cancel();
        }
        mTimer = new Timer();
        mTimer.scheduleAtFixedRate(
                new TimerTask() {
                    @Override
                    public void run() {
                        TimeDelta duration = new TimeDelta();
                        duration.microseconds = REPORT_INTERVAL_MS * 1000;
                        mMiscAndroidMetrics.recordBrowserUsageDuration(duration);
                    }
                },
                REPORT_INTERVAL_MS,
                REPORT_INTERVAL_MS);
    }

    public void stop() {
        mTimer.cancel();
        mTimer = null;
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks;

import android.content.Intent;
import android.os.Bundle;

import org.chromium.chrome.browser.ChromeInactivityTracker;
import org.chromium.chrome.browser.ntp.BraveFreshNtpHelper;

public final class BraveReturnToChromeUtil {
    /** Returns whether should show a NTP as the home surface at startup. */
    public static boolean shouldShowNtpAsHomeSurfaceAtStartup(
            Intent intent, Bundle bundle, ChromeInactivityTracker inactivityTracker) {
        // When feature is disabled, use Brave's default behavior
        if (!BraveFreshNtpHelper.isEnabled()) {
            return false;
        }

        String variant = BraveFreshNtpHelper.getVariant();
        switch (variant) {
            case "A":
                // Variant A: Brave's default behavior (no NTP at startup)
                return false;
            default:
                // All other variants: fallback to upstream behavior
                // return ReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                //        intent, bundle, inactivityTracker);
        }
        return false;
    }
}

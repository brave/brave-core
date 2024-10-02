/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks;

import android.content.Intent;
import android.os.Bundle;

import org.chromium.chrome.browser.ChromeInactivityTracker;

public final class BraveReturnToChromeUtil {
    /** Returns whether should show a NTP as the home surface at startup. */
    public static boolean shouldShowNtpAsHomeSurfaceAtStartup(
            Intent intent, Bundle bundle, ChromeInactivityTracker inactivityTracker) {
        // We do not want to show the NTP at start up in Brave.
        return false;
    }
}

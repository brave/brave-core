/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.safe_browsing.settings;

import android.content.Context;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.gms.ChromiumPlayServicesAvailability;

public class BraveSafeBrowsingSettingsFragment {
    public static String getSafeBrowsingSummaryString(Context context, Profile profile) {
        if (!ChromiumPlayServicesAvailability.isGooglePlayServicesAvailable(context)) {
            return context.getString(R.string.prefs_safe_browsing_no_protection_summary);
        }
        return SafeBrowsingSettingsFragment.getSafeBrowsingSummaryString(context, profile);
    }
}

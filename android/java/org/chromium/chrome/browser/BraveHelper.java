/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.flags.ChromeSwitches;

public class BraveHelper {
    public static final String SHARED_PREF_DISPLAYED_INFOBAR_PROMO =
            "displayed_data_reduction_infobar_promo";
    // Used to indicate were the settings migrated to the new
    // brave-core based version
    public static final String PREF_TABS_SETTINGS_MIGRATED =
            "android_tabs_settings_to_core_migrated";

    public BraveHelper() {}

    public static void DisableFREDRP() {
        // Disables data reduction promo dialog
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(SHARED_PREF_DISPLAYED_INFOBAR_PROMO, true)
                .apply();
    }
}

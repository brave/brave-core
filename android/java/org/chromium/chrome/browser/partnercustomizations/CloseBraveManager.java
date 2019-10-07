/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.partnercustomizations;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp.NewTabPage;

public class CloseBraveManager {
    private static final String CLOSING_ALL_TABS_CLOSES_BRAVE = "closing_all_tabs_closes_brave";

    // When NTP is set as home page, we don't close brave always.
    // Otherwise, follow option value.
    public static boolean shouldCloseAppWithZeroTabs() {
        if (HomepageManager.isHomepageEnabled() &&
                NewTabPage.isNTPUrl(HomepageManager.getHomepageUri())) {
            return false;
        }

        return getClosingAllTabsClosesBraveEnabled();
    }

    public static boolean getClosingAllTabsClosesBraveEnabled() {
        return ContextUtils.getAppSharedPreferences().getBoolean(CLOSING_ALL_TABS_CLOSES_BRAVE, false);
    }

    public static void setClosingAllTabsClosesBraveEnabled(boolean enable) {
        SharedPreferences.Editor sharedPreferencesEditor =
            ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(CLOSING_ALL_TABS_CLOSES_BRAVE, enable);
        sharedPreferencesEditor.apply();
    }
}

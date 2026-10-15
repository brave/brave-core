/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Brave's defaults for the bottom bar user setting. */
@NullMarked
public class BraveBottomBarUserPrefs {
    private BraveBottomBarUserPrefs() {}

    /**
     * Default for upstream's bottom bar setting until the user flips it: Brave's former bottom bar
     * setting, then the bottom navigation toolbar setting, so turning the flag on doesn't hand a
     * bottom bar back to someone who turned either off.
     */
    public static boolean getDefaultUserEnabled(boolean upstreamDefault) {
        SharedPreferencesManager prefs = ChromeSharedPreferences.getInstance();
        return prefs.readBoolean(
                BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR,
                prefs.readBoolean(
                        BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, upstreamDefault));
    }
}

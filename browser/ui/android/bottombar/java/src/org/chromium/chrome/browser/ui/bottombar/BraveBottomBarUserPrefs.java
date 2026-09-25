/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ResettersForTesting;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Brave's user setting for the bottom bar. */
@NullMarked
public class BraveBottomBarUserPrefs {
    private static @Nullable Boolean sBottomBarEnabled;

    private BraveBottomBarUserPrefs() {}

    /**
     * Whether the bottom bar is on for this run, i.e. {@link #isBottomBarSettingEnabled()} as it
     * was first read.
     *
     * <p>Frozen like the flag it sits next to: the bottom bar UI is built once per run, so a
     * setting change declined for relaunch must not leave callers disagreeing with that UI.
     */
    public static boolean isBottomBarEnabled() {
        if (sBottomBarEnabled == null) {
            sBottomBarEnabled = isBottomBarSettingEnabled();
            ResettersForTesting.register(() -> sBottomBarEnabled = null);
        }
        return sBottomBarEnabled;
    }

    /**
     * Whether the user has "Enable bottom bar" switched on in appearance settings right now.
     *
     * <p>The bottom bar and Brave's own bottom navigation controls are separate implementations of
     * the same thing, with a setting each, so this one only ever applies while the Android bottom
     * bar flag is on. Until it is set it inherits the bottom navigation toolbar setting, so
     * switching the flag on does not hand a bottom bar back to someone who turned that one off.
     */
    public static boolean isBottomBarSettingEnabled() {
        SharedPreferencesManager prefs = ChromeSharedPreferences.getInstance();
        return prefs.readBoolean(
                BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR,
                prefs.readBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, true));
    }
}

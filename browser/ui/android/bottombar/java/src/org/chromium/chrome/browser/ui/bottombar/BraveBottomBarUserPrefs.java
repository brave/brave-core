/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Brave's user setting for the bottom bar. */
@NullMarked
public class BraveBottomBarUserPrefs {
    private BraveBottomBarUserPrefs() {}

    /**
     * Whether the user has "Enable bottom bar" switched on in appearance settings.
     *
     * <p>The bottom bar and Brave's own bottom navigation controls are separate implementations of
     * the same thing, with a setting each, so this one only ever applies while the Android bottom
     * bar flag is on. Defaults to on, the state the bottom navigation toolbar setting starts in
     * too.
     */
    public static boolean isBottomBarEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR, true);
    }
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

@NullMarked
public class BraveTabUiFeatureUtilities {
    public static boolean isTabGroupsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
    }

    public static void setTabGroupsEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
    }

    public static boolean isBraveTabGroupsEnabled() {
        return isTabGroupsEnabled() && isOpenLinksInCurrentTabGroupEnabled();
    }

    public static boolean isOpenLinksInCurrentTabGroupEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED,
                        ChromeSharedPreferences.getInstance()
                                .readBoolean(
                                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE,
                                        true));
    }

    public static void setOpenLinksInCurrentTabGroupEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, enabled);
    }

    public static boolean isTabGroupsBarPreferenceEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, true);
    }

    public static void setTabGroupsBarEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, enabled);
    }
}

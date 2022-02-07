/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.annotation.SuppressLint;
import android.content.Context;

import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;

public class BraveTabUiFeatureUtilities {
    private static final String TAB_GROUP_AUTO_CREATION_PREFERENCE =
            "Chrome.Flags.FieldTrialParamCached.TabGridLayoutAndroid:enable_tab_group_auto_creation";

    /**
     * @return Whether the Grid Tab Switcher UI is enabled and available for use.
     */
    public static boolean isGridTabSwitcherEnabled(Context context) {
        if (!isTabGroupsAndroidEnabled(context)) {
            return false;
        }
        return TabUiFeatureUtilities.isGridTabSwitcherEnabled(context);
    }

    /**
     * @return Whether the tab group feature is enabled and available for use.
     */
    public static boolean isTabGroupsAndroidEnabled(Context context) {
        // For backward compatibility we take value of Tab Grid feature if BRAVE_TAB_GROUPS_ENABLED
        // setting hasn't been created. We don't want to rely on Tab Grid feature itself since it
        // can be removed in the upstream going forward.
        if (!SharedPreferencesManager.getInstance().contains(
                    BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED,
                    CachedFeatureFlags.isEnabled(ChromeFeatureList.TAB_GRID_LAYOUT_ANDROID));
        }

        if (!SharedPreferencesManager.getInstance().readBoolean(
                    BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, true)) {
            return false;
        }

        return TabUiFeatureUtilities.isTabGroupsAndroidEnabled(context);
    }

    @SuppressLint("VisibleForTests")
    public static void maybeOverrideEnableTabGroupAutoCreationPreference() {
        if (TabUiFeatureUtilities.ENABLE_TAB_GROUP_AUTO_CREATION.getValue()) {
            // Override it to make "Open in new tab" menu option in the context menu available.
            SharedPreferencesManager.getInstance().writeBoolean(
                    TAB_GROUP_AUTO_CREATION_PREFERENCE, false);
            CachedFeatureFlags.resetFlagsForTesting();
        }
    }
}

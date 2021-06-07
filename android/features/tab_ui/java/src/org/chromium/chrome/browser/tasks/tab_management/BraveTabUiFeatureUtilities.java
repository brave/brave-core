/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;

public class BraveTabUiFeatureUtilities {
    /**
     * @return Whether the Grid Tab Switcher UI is enabled and available for use.
     */
    public static boolean isGridTabSwitcherEnabled() {
        if (!isTabGroupsAndroidEnabled()) {
            return false;
        }
        return TabUiFeatureUtilities.isGridTabSwitcherEnabled();
    }

    /**
     * @return Whether the tab group feature is enabled and available for use.
     */
    public static boolean isTabGroupsAndroidEnabled() {
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

        return TabUiFeatureUtilities.isTabGroupsAndroidEnabled();
    }
}

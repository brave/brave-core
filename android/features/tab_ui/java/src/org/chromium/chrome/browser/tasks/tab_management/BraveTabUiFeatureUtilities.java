/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.annotation.SuppressLint;
import android.content.Context;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;

public class BraveTabUiFeatureUtilities {
    private static final String TAB_GROUP_AUTO_CREATION_PREFERENCE =
            "Chrome.Flags.FieldTrialParamCached.TabGridLayoutAndroid:enable_tab_group_auto_creation";

    @SuppressLint("VisibleForTests")
    public static void maybeOverrideEnableTabGroupAutoCreationPreference(Context context) {
        if (TabUiFeatureUtilities.isTabGroupsAndroidEnabled(context)) {
            // Override it to make "Open in new tab" menu option in the context menu available if
            // applicable.
            SharedPreferencesManager.getInstance().writeBoolean(
                    TAB_GROUP_AUTO_CREATION_PREFERENCE, isBraveTabGroupsEnabled());
            CachedFeatureFlags.resetFlagsForTesting();
        }
    }

    public static boolean isBraveTabGroupsEnabled() {
        return SharedPreferencesManager.getInstance().readBoolean(
                BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, true);
    }
}

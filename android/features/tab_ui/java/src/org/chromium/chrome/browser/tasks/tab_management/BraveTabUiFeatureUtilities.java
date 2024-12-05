/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class BraveTabUiFeatureUtilities {
    public static boolean isBraveTabGroupsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED,
                        ChromeSharedPreferences.getInstance()
                                .readBoolean(
                                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE,
                                        true));
    }
}

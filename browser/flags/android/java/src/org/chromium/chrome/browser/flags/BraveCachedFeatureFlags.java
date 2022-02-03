/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.flags;

public class BraveCachedFeatureFlags {
    private static final String TAB_GROUP_AUTO_CREATION_PREF =
            "TabGridLayoutAndroid:enable_tab_group_auto_creation";

    static boolean getConsistentBooleanValue(String preferenceName, boolean defaultValue) {
        if (preferenceName.endsWith(TAB_GROUP_AUTO_CREATION_PREF)) {
            return false;
        }

        return CachedFeatureFlags.getConsistentBooleanValue(preferenceName, defaultValue);
    }
}

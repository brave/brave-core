/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Intent;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;

// see org.brave.bytecode.BraveLaunchIntentDispatcherClassAdapter
public class BraveLaunchIntentDispatcher {
    public static boolean isCustomTabIntent(Intent intent) {
        if (!useCustomTabs()) {
            return false;
        }
        return LaunchIntentDispatcher.isCustomTabIntent(intent);
    }

    public static boolean useCustomTabs() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BravePreferenceKeys.BRAVE_USE_CUSTOM_TABS, true);
    }
}

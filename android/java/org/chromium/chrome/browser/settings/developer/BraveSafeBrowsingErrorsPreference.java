/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.developer;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

public class BraveSafeBrowsingErrorsPreference extends ChromeSwitchPreference {
    public BraveSafeBrowsingErrorsPreference(Context context) {
        super(context);
    }

    public BraveSafeBrowsingErrorsPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onClick() {
        super.onClick();
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_SAFE_BROWSING_ERRORS, isChecked());
    }
}

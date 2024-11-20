/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.accessibility.settings;

import android.os.Bundle;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.components.browser_ui.accessibility.AccessibilitySettings;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

public class BraveAccessibilitySettings extends AccessibilitySettings {
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        ChromeSwitchPreference readerForAccessibilityPref =
                (ChromeSwitchPreference) findPreference(PREF_READER_FOR_ACCESSIBILITY);
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SPEEDREADER)) {
            getPreferenceScreen().removePreference(readerForAccessibilityPref);
        }
    }
}

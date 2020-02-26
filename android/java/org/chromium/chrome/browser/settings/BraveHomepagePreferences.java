/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import org.chromium.chrome.browser.partnercustomizations.HomepageManager;

public class BraveHomepagePreferences extends HomepagePreferences {
    private HomepageManager mHomepageManager;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        mHomepageManager = HomepageManager.getInstance();
        ChromeSwitchPreference homepageSwitch =
                (ChromeSwitchPreference) findPreference(PREF_HOMEPAGE_SWITCH);

        if (homepageSwitch.isVisible()) return;
        // Show homepage switch if it is hidden.
        homepageSwitch.setVisible(true);
        boolean isHomepageEnabled = mHomepageManager.getPrefHomepageEnabled();
        homepageSwitch.setChecked(isHomepageEnabled);
        homepageSwitch.setOnPreferenceChangeListener((preference, newValue) -> {
            mHomepageManager.setPrefHomepageEnabled((boolean) newValue);
            return true;
        });
    }
}

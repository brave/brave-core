/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

public class BraveSiteSettingsPreferences extends SiteSettingsPreferences {
    private static final String DESKTOP_MODE_CATEGORY_KEY = "desktop_mode_category";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.brave_site_settings_preferences);
        updateBravePreferenceStates();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateBravePreferenceStates();
    }

    private void updateBravePreferenceStates() {
        Preference p = findPreference(DESKTOP_MODE_CATEGORY_KEY);
        boolean enabled = ContextUtils.getAppSharedPreferences().getBoolean(
             DesktopModePreferences.DESKTOP_MODE_KEY, false);
        p.setSummary(enabled ? R.string.settings_desktop_mode_enabled_summary
                             : R.string.settings_desktop_mode_disabled_summary);
    }
}

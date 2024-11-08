/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.components.browser_ui.settings.SettingsUtils;

import java.util.HashMap;

public class BraveSiteSettingsPreferencesBase extends BaseSiteSettingsFragment {
    private static final String ADS_KEY = "ads";
    private static final String BACKGROUND_SYNC_KEY = "background_sync";
    private static final String IDLE_DETECTION = "idle_detection";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (SiteSettingsPreferences::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_site_settings_preferences);
        configureBravePreferences();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    @Override
    public void onResume() {
        super.onResume();
    }

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public Preference findPreference(CharSequence key) {
        Preference result = super.findPreference(key);
        if (result == null) {
            result = mRemovedPreferences.get((String) key);
        }
        return result;
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }

    private void configureBravePreferences() {
        removePreferenceIfPresent(IDLE_DETECTION);
        removePreferenceIfPresent(ADS_KEY);
        removePreferenceIfPresent(BACKGROUND_SYNC_KEY);
    }
}

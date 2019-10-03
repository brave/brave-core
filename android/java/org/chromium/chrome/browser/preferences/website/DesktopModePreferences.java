/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreferenceCompat;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

public class DesktopModePreferences
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    public static final String DESKTOP_MODE_KEY = "desktop_mode";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.settings_desktop_mode_title);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.desktop_mode_preferences);

        ChromeSwitchPreferenceCompat desktopModePref =
                (ChromeSwitchPreferenceCompat) findPreference(DESKTOP_MODE_KEY);
        desktopModePref.setChecked(BravePrefServiceBridge.getInstance().getDesktopModeEnabled());
        desktopModePref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        BravePrefServiceBridge.getInstance().setDesktopModeEnabled((boolean) newValue);
        return true;
    }
}

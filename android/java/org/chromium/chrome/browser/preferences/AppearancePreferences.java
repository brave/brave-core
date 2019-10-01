/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreference;
import org.chromium.ui.base.DeviceFormFactor;

public class AppearancePreferences
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_appearance);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.appearance_preferences);
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                ContextUtils.getApplicationContext());
        if (isTablet) {
            removePreferenceIfPresent(ChromePreferenceManager.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) getPreferenceScreen().removePreference(preference);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        Preference enableBottomToolbar =
                findPreference(ChromePreferenceManager.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        if (enableBottomToolbar == null) return;

        enableBottomToolbar.setOnPreferenceChangeListener(this);
        if (enableBottomToolbar instanceof ChromeSwitchPreference) {
            boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                    ContextUtils.getApplicationContext());
            ((ChromeSwitchPreference) enableBottomToolbar)
                    .setChecked(!isTablet
                            && ChromePreferenceManager.getInstance().isBottomToolbarEnabled());
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (ChromePreferenceManager.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY.equals(key)) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Boolean originalStatus = ChromePreferenceManager.getInstance().isBottomToolbarEnabled();
            prefs.edit()
                    .putBoolean(ChromePreferenceManager.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                            !originalStatus)
                    .apply();
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }
        return true;
    }
}

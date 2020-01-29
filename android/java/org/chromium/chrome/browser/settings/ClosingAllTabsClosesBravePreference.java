/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.ChromeSwitchPreference;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;

public class ClosingAllTabsClosesBravePreference
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    private static final String CLOSING_ALL_TABS_CLOSES_BRAVE_KEY = "closing_all_tabs_closes_brave";

    public static int getPreferenceSummary() {
        return CloseBraveManager.getClosingAllTabsClosesBraveEnabled() ? R.string.text_on : R.string.text_off;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.prefs_closing_all_tabs_closes_brave);
        SettingsUtils.addPreferencesFromResource(this, R.xml.closing_all_tabs_closes_brave_preference);

        ChromeSwitchPreference pref =
                (ChromeSwitchPreference) findPreference(CLOSING_ALL_TABS_CLOSES_BRAVE_KEY);
        pref.setChecked(CloseBraveManager.getClosingAllTabsClosesBraveEnabled());
        pref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        CloseBraveManager.setClosingAllTabsClosesBraveEnabled((boolean) newValue);
        return true;
    }
}

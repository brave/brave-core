/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import static org.chromium.chrome.browser.settings.MainPreferences.PREF_UI_THEME;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.night_mode.NightModeUtils;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.ChromeSwitchPreference;
import org.chromium.chrome.browser.flags.FeatureUtilities;
import org.chromium.ui.base.DeviceFormFactor;

public class AppearancePreferences
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    public static final String PREF_HIDE_BRAVE_REWARDS_ICON = "hide_brave_rewards_icon";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_appearance);
        SettingsUtils.addPreferencesFromResource(this, R.xml.appearance_preferences);
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                ContextUtils.getApplicationContext());
        if (isTablet) {
            removePreferenceIfPresent(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        }

        if (!NightModeUtils.isNightModeSupported() || !FeatureUtilities.isNightModeAvailable()) {
            removePreferenceIfPresent(PREF_UI_THEME);
        }

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)) {
            removePreferenceIfPresent(PREF_HIDE_BRAVE_REWARDS_ICON);
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
                findPreference(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        if (enableBottomToolbar == null) return;

        enableBottomToolbar.setOnPreferenceChangeListener(this);
        if (enableBottomToolbar instanceof ChromeSwitchPreference) {
            boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                    ContextUtils.getApplicationContext());
            ((ChromeSwitchPreference) enableBottomToolbar)
                    .setChecked(!isTablet
                            && FeatureUtilities.isBottomToolbarEnabled());
        }

        Preference hideBraveRewardsIconPref = findPreference(PREF_HIDE_BRAVE_REWARDS_ICON);
        if (hideBraveRewardsIconPref != null) {
            // TODO(jocelyn): Do setEnabled(false) here when
            // BraveRewardsNativeWorker is implemented.
            hideBraveRewardsIconPref.setOnPreferenceChangeListener(this);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY.equals(key)) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Boolean originalStatus = FeatureUtilities.isBottomToolbarEnabled();
            prefs.edit()
                    .putBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                            !originalStatus)
                    .apply();
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else if (PREF_HIDE_BRAVE_REWARDS_ICON.equals(key)) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(PREF_HIDE_BRAVE_REWARDS_ICON, (boolean) newValue);
            sharedPreferencesEditor.apply();
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }
        return true;
    }
}

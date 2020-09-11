/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * Fragment to keep track of all the display related preferences.
 */
public class BackgroundImagesPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener {

    // deprecated preferences from browser-android-tabs
    public static final String PREF_SHOW_BACKGROUND_IMAGES = "show_background_images";
    public static final String PREF_SHOW_SPONSORED_IMAGES = "show_sponsored_images";
    public static final String PREF_SHOW_NON_DISTRUPTIVE_BANNER = "show_non_distruptive_banner";

    private ChromeSwitchPreference showBackgroundImagesPref;
    private ChromeSwitchPreference showSponsoredImagesPref;

    private SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
    private SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_new_tab_page);
        SettingsUtils.addPreferencesFromResource(this, R.xml.background_images_preferences);
        if (!NTPBackgroundImagesBridge.enableSponsoredImages()) {
            removePreferenceIfPresent(PREF_SHOW_SPONSORED_IMAGES);
        }
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) getPreferenceScreen().removePreference(preference);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        showBackgroundImagesPref = (ChromeSwitchPreference) findPreference(PREF_SHOW_BACKGROUND_IMAGES);
        if (showBackgroundImagesPref != null) {
            showBackgroundImagesPref.setEnabled(true);
            showBackgroundImagesPref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE));
            showBackgroundImagesPref.setOnPreferenceChangeListener(this);
        }
        showSponsoredImagesPref = (ChromeSwitchPreference) findPreference(PREF_SHOW_SPONSORED_IMAGES);
        if (showSponsoredImagesPref != null) {
            showSponsoredImagesPref.setEnabled(UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE));
            showSponsoredImagesPref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.NEW_TAB_PAGE_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE));
            showSponsoredImagesPref.setOnPreferenceChangeListener(this);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_SHOW_BACKGROUND_IMAGES.equals(preference.getKey()) && showSponsoredImagesPref != null) {
            showSponsoredImagesPref.setEnabled((boolean)newValue);
        }
        setOnPreferenceValue(preference.getKey(), (boolean)newValue);
        BraveRelaunchUtils.askForRelaunch(getActivity());
        return true;
    }

    public static void setOnPreferenceValue(String preferenceName, boolean newValue) {
        if (PREF_SHOW_BACKGROUND_IMAGES.equals(preferenceName)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE, newValue);
        } else if (PREF_SHOW_SPONSORED_IMAGES.equals(preferenceName)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(BravePref.NEW_TAB_PAGE_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE, (boolean)newValue);
        } else {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(preferenceName, newValue);
            sharedPreferencesEditor.apply();
        }
    }

    public static void setOnPreferenceValue(String preferenceName, int newValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putInt(preferenceName, newValue);
        sharedPreferencesEditor.apply();
    }
}

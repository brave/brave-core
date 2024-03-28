/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.PreferenceCategory;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * Fragment to keep track of all the display related preferences.
 */
public class BackgroundImagesPreferences
        extends BravePreferenceFragment implements OnPreferenceChangeListener {
    // deprecated preferences from browser-android-tabs
    public static final String PREF_SHOW_BACKGROUND_IMAGES = "show_background_images";
    public static final String PREF_SHOW_SPONSORED_IMAGES = "show_sponsored_images";
    public static final String PREF_SHOW_TOP_SITES = "show_top_sites";
    public static final String PREF_SHOW_BRAVE_STATS = "show_brave_stats";
    public static final String PREF_SHOW_NON_DISRUPTIVE_BANNER = "show_non_disruptive_banner";
    public static final String PREF_SHOW_BRE_BANNER = "show_bre_banner";
    public static final String PREF_BACKGROUND_IMAGES_CATEGORY = "background_images";

    private ChromeSwitchPreference mShowBackgroundImagesPref;
    private ChromeSwitchPreference mShowSponsoredImagesPref;
    private ChromeSwitchPreference mShowBraveStatsPref;
    private ChromeSwitchPreference mShowTopSitesPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_new_tab_page);
        SettingsUtils.addPreferencesFromResource(this, R.xml.background_images_preferences);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mShowBackgroundImagesPref =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_BACKGROUND_IMAGES);
        if (mShowBackgroundImagesPref != null) {
            mShowBackgroundImagesPref.setEnabled(true);
            mShowBackgroundImagesPref.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE));
            mShowBackgroundImagesPref.setOnPreferenceChangeListener(this);
        }
        mShowSponsoredImagesPref =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_SPONSORED_IMAGES);
        if (mShowSponsoredImagesPref != null) {
            mShowSponsoredImagesPref.setEnabled(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE));
            mShowSponsoredImagesPref.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(
                                    BravePref.NEW_TAB_PAGE_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE));
            mShowSponsoredImagesPref.setOnPreferenceChangeListener(this);
        }
        if (!NTPBackgroundImagesBridge.enableSponsoredImages()) {
            PreferenceCategory preferenceCategory =
                    (PreferenceCategory) findPreference(PREF_BACKGROUND_IMAGES_CATEGORY);
            preferenceCategory.removePreference(mShowSponsoredImagesPref);
        }

        mShowTopSitesPref = (ChromeSwitchPreference) findPreference(PREF_SHOW_TOP_SITES);
        if (mShowTopSitesPref != null) {
            mShowTopSitesPref.setEnabled(true);
            mShowTopSitesPref.setChecked(
                    ContextUtils.getAppSharedPreferences().getBoolean(PREF_SHOW_TOP_SITES, true));
            mShowTopSitesPref.setOnPreferenceChangeListener(this);
        }
        mShowBraveStatsPref = (ChromeSwitchPreference) findPreference(PREF_SHOW_BRAVE_STATS);
        if (mShowBraveStatsPref != null) {
            mShowBraveStatsPref.setEnabled(true);
            mShowBraveStatsPref.setChecked(
                    ContextUtils.getAppSharedPreferences().getBoolean(PREF_SHOW_BRAVE_STATS, true));
            mShowBraveStatsPref.setOnPreferenceChangeListener(this);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_SHOW_BACKGROUND_IMAGES.equals(preference.getKey())
                && mShowSponsoredImagesPref != null) {
            mShowSponsoredImagesPref.setEnabled((boolean) newValue);
        }
        setOnPreferenceValue(preference.getKey(), (boolean)newValue);
        BraveRelaunchUtils.askForRelaunch(getActivity());
        return true;
    }

    public static void setOnPreferenceValue(String preferenceName, boolean newValue) {
        if (PREF_SHOW_BACKGROUND_IMAGES.equals(preferenceName)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile()).setBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE, newValue);
        } else if (PREF_SHOW_SPONSORED_IMAGES.equals(preferenceName)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile()).setBoolean(BravePref.NEW_TAB_PAGE_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE, (boolean)newValue);
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

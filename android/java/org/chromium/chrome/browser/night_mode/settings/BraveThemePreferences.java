/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.night_mode.settings;

import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.UI_THEME_DARKEN_WEBSITES_ENABLED;
import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.UI_THEME_SETTING;

import android.os.Build;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.Preference;

import org.chromium.base.BuildInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.night_mode.ThemeType;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveThemePreferences extends ThemeSettingsFragment {

    private static final String SUPER_REFERRAL = "super_referral";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_theme_preferences);
        getActivity().setTitle(getResources().getString(R.string.theme_settings));

        Profile mProfile = Profile.getLastUsedRegularProfile();
        NTPBackgroundImagesBridge mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        if (!NTPBackgroundImagesBridge.enableSponsoredImages()
                || (mNTPBackgroundImagesBridge != null
                    && !mNTPBackgroundImagesBridge.isSuperReferral())
                || Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP) {
            Preference superReferralPreference = getPreferenceScreen().findPreference(SUPER_REFERRAL);
            if (superReferralPreference != null) {
                getPreferenceScreen().removePreference(superReferralPreference);
            }
        }

        SharedPreferencesManager sharedPreferencesManager = SharedPreferencesManager.getInstance();
        BraveRadioButtonGroupThemePreference radioButtonGroupThemePreference =
            (BraveRadioButtonGroupThemePreference) findPreference(PREF_UI_THEME_PREF);

        int defaultThemePref = ThemeType.SYSTEM_DEFAULT;
        if (!BuildInfo.isAtLeastQ()) {
            defaultThemePref = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                               ? ThemeType.DARK
                               : ThemeType.LIGHT;
        }
        radioButtonGroupThemePreference.initialize(
            sharedPreferencesManager.readInt(UI_THEME_SETTING, defaultThemePref),
            sharedPreferencesManager.readBoolean(UI_THEME_DARKEN_WEBSITES_ENABLED, false));

        radioButtonGroupThemePreference.setOnPreferenceChangeListener((preference, newValue) -> {
            if (ChromeFeatureList.isEnabled(
                ChromeFeatureList.DARKEN_WEBSITES_CHECKBOX_IN_THEMES_SETTING)) {
                sharedPreferencesManager.writeBoolean(UI_THEME_DARKEN_WEBSITES_ENABLED,
                radioButtonGroupThemePreference.isDarkenWebsitesEnabled());
            }
            int theme = (int) newValue;
            sharedPreferencesManager.writeInt(UI_THEME_SETTING, theme);
            return true;
        });
    }
}

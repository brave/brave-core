/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.themes;

import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.DARKEN_WEBSITES_ENABLED_KEY;
import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.UI_THEME_SETTING_KEY;

import android.os.Bundle;
import android.support.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.base.BuildInfo;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.SettingsUtils;

public class BraveThemePreferences extends ThemePreferences {
    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_theme_preferences);
        getActivity().setTitle(getResources().getString(R.string.prefs_themes));

        SharedPreferencesManager sharedPreferencesManager = SharedPreferencesManager.getInstance();
        BraveRadioButtonGroupThemePreference radioButtonGroupThemePreference =
                (BraveRadioButtonGroupThemePreference) findPreference(PREF_UI_THEME_PREF);

        int defaultThemePref = ThemeSetting.SYSTEM_DEFAULT;
        if (!BuildInfo.isAtLeastQ()) {
            defaultThemePref = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                    ? ThemeSetting.DARK
                    : ThemeSetting.LIGHT;
        }
        radioButtonGroupThemePreference.initialize(
                sharedPreferencesManager.readInt(UI_THEME_SETTING_KEY, defaultThemePref),
                sharedPreferencesManager.readBoolean(DARKEN_WEBSITES_ENABLED_KEY, false));

        radioButtonGroupThemePreference.setOnPreferenceChangeListener((preference, newValue) -> {
            if (ChromeFeatureList.isEnabled(
                        ChromeFeatureList.DARKEN_WEBSITES_CHECKBOX_IN_THEMES_SETTING)) {
                sharedPreferencesManager.writeBoolean(DARKEN_WEBSITES_ENABLED_KEY,
                        radioButtonGroupThemePreference.isDarkenWebsitesEnabled());
            }
            int theme = (int) newValue;
            sharedPreferencesManager.writeInt(UI_THEME_SETTING_KEY, theme);
            return true;
        });
    }
}

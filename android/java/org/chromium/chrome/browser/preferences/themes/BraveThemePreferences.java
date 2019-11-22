/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.themes;

import static org.chromium.chrome.browser.preferences.ChromePreferenceManager.UI_THEME_SETTING_KEY;

import android.os.Bundle;
import android.support.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.base.BuildInfo;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

public class BraveThemePreferences extends ThemePreferences {
    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        PreferenceUtils.addPreferencesFromResource(this, R.xml.brave_theme_preferences);
        getActivity().setTitle(getResources().getString(R.string.prefs_themes));

        BraveRadioButtonGroupThemePreference radioButtonGroupThemePreference =
                (BraveRadioButtonGroupThemePreference) findPreference(PREF_UI_THEME_PREF);

        int defaultThemePref = ThemeSetting.SYSTEM_DEFAULT;
        if (!BuildInfo.isAtLeastQ()) {
            defaultThemePref = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                    ? ThemeSetting.DARK
                    : ThemeSetting.LIGHT;
        }
        radioButtonGroupThemePreference.initialize(ChromePreferenceManager.getInstance().readInt(
                UI_THEME_SETTING_KEY, defaultThemePref));

        radioButtonGroupThemePreference.setOnPreferenceChangeListener((preference, newValue) -> {
            int theme = (int) newValue;
            ChromePreferenceManager.getInstance().writeInt(UI_THEME_SETTING_KEY, theme);
            return true;
        });
    }
}

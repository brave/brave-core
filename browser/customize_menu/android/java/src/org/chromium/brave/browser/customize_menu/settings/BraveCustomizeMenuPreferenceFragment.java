/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu.settings;

import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.brave.browser.customize_menu.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.SettingsUtils;

/**
 * Customize menu preference settings fragment where a user can toggle the visibility of supported
 * items from main menu.
 */
@NullMarked
public class BraveCustomizeMenuPreferenceFragment extends ChromeBaseSettingsFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "CustomizeMenuPreferenceFragment";

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPageTitle.set(getString(R.string.customize_menu_title));
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_customize_menu_preferences);

        PreferenceCategory mainMenuSection = findPreference("main_menu_section");
        if (mainMenuSection != null) {
            //            ChromeSwitchPreference braveTranslateFeaturePreference =
            //                    new ChromeSwitchPreference(getContext());
            //            braveTranslateFeaturePreference.setTitle(
            //                    getResources().getString(R.string.use_brave_translate));
            //            braveTranslateFeaturePreference.setChecked(true);
            //            braveTranslateFeaturePreference.setIcon(R.drawable.ic_brave_ai);
            //            braveTranslateFeaturePreference.setOnPreferenceChangeListener(this);
            //            mainMenuSection.addPreference(braveTranslateFeaturePreference);
        }
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return false;
    }
}

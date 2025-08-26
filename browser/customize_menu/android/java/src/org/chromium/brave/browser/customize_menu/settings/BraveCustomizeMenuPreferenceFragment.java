/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu.settings;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.brave.browser.customize_menu.MenuItemData;
import org.chromium.brave.browser.customize_menu.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

import java.util.List;
import java.util.Locale;

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
            // Get menu items from bundle passed by calling activity.
            final Bundle bundle = getArguments();
            if (bundle != null) {
                List<MenuItemData> menuItems = bundle.getParcelableArrayList("menu_items");
                if (menuItems != null) {
                    for (MenuItemData menuItem : menuItems) {
                        if (menuItem.id == R.id.brave_customize_menu_id) {
                            continue;
                        }
                        ChromeSwitchPreference switchPreference = getSwitchPreference(menuItem);
                        mainMenuSection.addPreference(switchPreference);
                    }
                }
            }
        }
    }

    private ChromeSwitchPreference getSwitchPreference(MenuItemData menuItem) {
        ChromeSwitchPreference preference = new ChromeSwitchPreference(getContext());
        preference.setTitle(menuItem.title);
        preference.setKey(
                String.format(Locale.ENGLISH, CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT, menuItem.id));
        preference.setChecked(menuItem.checked);

        // Set icon if available
        if (menuItem.iconResId != 0) {
            preference.setIcon(menuItem.iconResId);
        }

        preference.setOnPreferenceChangeListener(this);
        return preference;
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
        ChromeSharedPreferences.getInstance().writeBoolean(preference.getKey(), (Boolean) newValue);
        return true;
    }
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu.settings;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.res.Resources;
import android.os.Bundle;

import androidx.annotation.DrawableRes;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.brave.browser.customize_menu.CustomizeBraveMenu;
import org.chromium.brave.browser.customize_menu.MenuItemData;
import org.chromium.brave.browser.customize_menu.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.base.ViewUtils;

import java.util.List;
import java.util.Locale;

/**
 * Customize menu preference settings fragment where a user can toggle the visibility of supported
 * items from main menu.
 */
@NullMarked
public class BraveCustomizeMenuPreferenceFragment extends ChromeBaseSettingsFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String MAIN_MENU_SECTION = "main_menu_section";
    private static final String PAGE_ACTIONS_SECTION = "page_actions_section";

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();
    private int mIconSizePx;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPageTitle.set(getString(R.string.customize_menu_title));
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_customize_menu_preferences);
        mIconSizePx =
                ViewUtils.dpToPx(requireContext(), CustomizeBraveMenu.PREFERENCE_MENU_ICON_SIZE_DP);
        // Get menu items from bundle passed by calling activity.
        final Bundle bundle = getArguments();
        if (bundle != null) {
            final PreferenceCategory mainMenuSection = findPreference(MAIN_MENU_SECTION);
            final PreferenceCategory pageActionsSection = findPreference(PAGE_ACTIONS_SECTION);
            addSwitchesFromBundle(
                    mainMenuSection, bundle, CustomizeBraveMenu.KEY_MAIN_MENU_ITEM_LIST);
            addSwitchesFromBundle(
                    pageActionsSection, bundle, CustomizeBraveMenu.KEY_PAGE_ACTION_ITEM_LIST);
        }
    }

    private void addSwitchesFromBundle(
            @Nullable final PreferenceCategory menuSection,
            final Bundle bundle,
            final String keyMenuItemList) {
        if (menuSection == null) {
            return;
        }
        final List<MenuItemData> menuItems = bundle.getParcelableArrayList(keyMenuItemList);
        if (menuItems != null) {
            for (MenuItemData menuItem : menuItems) {
                ChromeSwitchPreference switchPreference = getSwitchPreference(menuItem);
                if (switchPreference != null) {
                    menuSection.addPreference(switchPreference);
                }
            }
        }
    }

    @Nullable
    private ChromeSwitchPreference getSwitchPreference(MenuItemData menuItem) {
        String resourceName;
        try {
            resourceName = getResources().getResourceEntryName(menuItem.id);
        } catch (Resources.NotFoundException notFoundException) {
            assert false : "Resource not found for menu item with ID " + menuItem.id;
            return null;
        }
        ChromeSwitchPreference preference = new ChromeSwitchPreference(getContext());
        preference.setTitle(menuItem.title);
        preference.setKey(
                String.format(
                        Locale.ENGLISH, CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT, resourceName));
        preference.setChecked(menuItem.checked);
        preference.setIconSpaceReserved(true);

        @DrawableRes int drawableRes = CustomizeBraveMenu.getDrawableResFromMenuItemId(menuItem.id);
        CustomizeBraveMenu.getStandardizedMenuIconAsync(
                requireContext(),
                drawableRes,
                mIconSizePx,
                drawable -> {
                    if (drawable != null) {
                        preference.setIcon(drawable);
                    }
                });

        preference.setOnPreferenceChangeListener(this);
        return preference;
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
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

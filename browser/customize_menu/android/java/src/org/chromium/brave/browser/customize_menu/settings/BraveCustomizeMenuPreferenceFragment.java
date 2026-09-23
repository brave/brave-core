/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu.settings;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.res.Resources;
import android.os.Bundle;

import androidx.annotation.DrawableRes;
import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.brave.browser.customize_menu.CustomizeBraveMenu;
import org.chromium.brave.browser.customize_menu.MenuItemData;
import org.chromium.brave.browser.customize_menu.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.ui.base.ViewUtils;

import java.util.ArrayList;
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

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();
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
        // Check the type first: SettingsIndexData's JSON serialization (via RecentSearchQueue)
        // stores Parcelable ArrayLists as Strings. Calling getParcelableArrayList on a String
        // value returns null on real Android (with a warning) but throws ClassCastException in
        // Robolectric. The instanceof guard avoids both.
        List<MenuItemData> menuItems =
                bundle.get(keyMenuItemList) instanceof ArrayList
                        ? bundle.getParcelableArrayList(keyMenuItemList)
                        : null;
        if (menuItems == null) {
            // SettingsIndexData serializes extras to JSON via RecentSearchQueue, which only
            // supports simple types — Parcelable ArrayLists are stringified and lost. Fall back
            // to the last live bundle from populateBundle.
            Bundle fallback = CustomizeBraveMenu.getBundleForSearchResults();
            if (fallback != null) {
                menuItems = fallback.getParcelableArrayList(keyMenuItemList);
            }
        }
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

    @VisibleForTesting
    public static Bundle getSearchExtrasForTesting() {
        return SEARCH_INDEX_DATA_PROVIDER.getExtras();
    }

    // All menu item switches are added programmatically; there are no static preferences to index.
    // This fragment is surfaced in search via BraveMainPreferencesBase.SEARCH_INDEX_DATA_PROVIDER.
    // getExtras() returns the last live bundle so that if this provider is ever consulted directly,
    // the correct extras are available.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveCustomizeMenuPreferenceFragment.class.getName(),
                    BaseSearchIndexProvider.INDEX_OPT_OUT) {
                @Override
                public Bundle getExtras() {
                    Bundle bundle = CustomizeBraveMenu.getBundleForSearchResults();
                    return bundle != null ? bundle : new Bundle();
                }
            };
}

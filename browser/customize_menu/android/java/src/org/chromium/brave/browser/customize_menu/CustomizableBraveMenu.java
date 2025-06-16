/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.Context;
import android.view.Menu;
import android.view.MenuItem;

import androidx.annotation.IdRes;

import org.chromium.brave.browser.customize_menu.settings.CustomizeMenuPreferenceFragment;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.browser_ui.settings.SettingsNavigation;

import java.util.Locale;

/**
 * A Brave menu to handle customization logic for all those items that can be hidden from Settings >
 * Appearance > Customize menu. See {@link CustomizeMenuPreferenceFragment} to inspect the settings
 * screen where a user can toggle the visibility of supported items.
 */
@NullMarked
public class CustomizableBraveMenu {
    private final Menu mMenu;

    public CustomizableBraveMenu(final Menu menu) {
        mMenu = menu;
    }

    public void applyCustomization() {
        android.util.Log.d("SIMONE", "applyCustomization");

        for (int i = 0; i < mMenu.size(); i++) {
            MenuItem menuItem = mMenu.getItem(i);
            // Skip current item if already invisible.
            if (!menuItem.isVisible()) {
                continue;
            }
            boolean visible =
                    ChromeSharedPreferences.getInstance()
                            .readBoolean(
                                    String.format(
                                            Locale.ENGLISH,
                                            CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT,
                                            menuItem.getItemId()),
                                    true);
            menuItem.setVisible(visible);

            int itemId = menuItem.getItemId();
            String title = menuItem.getTitle() != null ? menuItem.getTitle().toString() : "";
            android.util.Log.d(
                    "SIMONE", "Menu Item: " + title + ", ID: " + itemId + ", visible: " + visible);
        }
    }

    public void removeItem(@IdRes int id) {
        mMenu.removeItem(id);
    }

    public static void openCustomizeMenuSettings(final Context context) {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(context, CustomizeMenuPreferenceFragment.class);
    }
}

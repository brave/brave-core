/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.Context;

import androidx.annotation.IdRes;

import org.chromium.brave.browser.customize_menu.settings.CustomizeMenuPreferenceFragment;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.ui.modelutil.MVCListAdapter;

import java.util.Iterator;
import java.util.Locale;

/**
 * A Brave menu to handle customization logic for all those items that can be hidden from Settings >
 * Appearance > Customize menu. See {@link CustomizeMenuPreferenceFragment} to inspect the settings
 * screen where a user can toggle the visibility of supported items.
 */
@NullMarked
public class CustomizableBraveMenu {

    public static void applyCustomization(
            final MVCListAdapter.ModelList modelList, @IdRes int customizeMenuId) {
        for (Iterator<MVCListAdapter.ListItem> it = modelList.iterator(); it.hasNext(); ) {
            MVCListAdapter.ListItem item = it.next();
            // Skip current item it matches the customize menu id.
            if (customizeMenuId == item.model.get(AppMenuItemProperties.MENU_ITEM_ID)) {
                continue;
            }
            boolean visible =
                    ChromeSharedPreferences.getInstance()
                            .readBoolean(
                                    String.format(
                                            Locale.ENGLISH,
                                            CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT,
                                            item.model.get(AppMenuItemProperties.MENU_ITEM_ID)),
                                    true);
            if (!visible) {
                it.remove();
            }
        }
    }

    public static void openCustomizeMenuSettings(final Context context) {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(context, CustomizeMenuPreferenceFragment.class);
    }
}

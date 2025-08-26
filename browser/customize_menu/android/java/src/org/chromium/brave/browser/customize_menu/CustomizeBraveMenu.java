/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import androidx.annotation.IdRes;

import org.chromium.brave.browser.customize_menu.settings.BraveCustomizeMenuPreferenceFragment;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.ui.modelutil.MVCListAdapter;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Locale;

/**
 * A Brave menu to handle customization logic for all those items that can be hidden from Settings >
 * Appearance > Customize menu. See {@link BraveCustomizeMenuPreferenceFragment} to inspect the
 * settings screen where a user can toggle the visibility of supported items.
 */
@NullMarked
public class CustomizeBraveMenu {

    @IdRes public static final int BRAVE_CUSTOMIZE_ITEM_ID = R.id.brave_customize_menu_id;

    public static void applyCustomization(final MVCListAdapter.ModelList modelList) {
        for (Iterator<MVCListAdapter.ListItem> it = modelList.iterator(); it.hasNext(); ) {
            MVCListAdapter.ListItem item = it.next();
            // Skip current item it matches the customize menu id.
            if (BRAVE_CUSTOMIZE_ITEM_ID == item.model.get(AppMenuItemProperties.MENU_ITEM_ID)) {
                continue;
            }
            boolean visible = isVisible(item.model.get(AppMenuItemProperties.MENU_ITEM_ID));
            if (!visible) {
                it.remove();
//                item.model.set(AppMenuItemProperties.ENABLED, false);
            }
        }
    }

    //    public static void openCustomizeMenuSettings(final Context context) {
    //        SettingsNavigation settingsLauncher =
    // SettingsNavigationFactory.createSettingsNavigation();
    //        settingsLauncher.startSettings(context, BraveCustomizeMenuPreferenceFragment.class);
    //    }

    public static void openCustomizeMenuSettings(
            final Context context, final MVCListAdapter.ModelList menuItems) {
        final Bundle bundle = new Bundle();

        // Convert menu items to parcelable data.
        final ArrayList<MenuItemData> menuItemDataList = new ArrayList<>();
        for (int i = 0; i < menuItems.size(); i++) {
            MVCListAdapter.ListItem item = menuItems.get(i);

            if (item.type == AppMenuHandler.AppMenuItemType.DIVIDER) {
                continue;
            }
            int id = item.model.get(AppMenuItemProperties.MENU_ITEM_ID);
            CharSequence title = item.model.get(AppMenuItemProperties.TITLE);
            Drawable icon = item.model.get(AppMenuItemProperties.ICON);

            // Convert drawable to resource ID (this is a limitation - we can only pass resource
            // IDs)
            int iconResId = 0;
            if (icon != null && icon instanceof android.graphics.drawable.BitmapDrawable) {
                // For now, we'll pass 0 for non-resource drawables
                // In a full implementation, you'd need a way to identify resource IDs from
                // drawables
            }

            menuItemDataList.add(
                    new MenuItemData(
                            id, title.toString(), iconResId, isVisible(id)));
        }

        bundle.putParcelableArrayList("menu_items", menuItemDataList);

        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(context, BraveCustomizeMenuPreferenceFragment.class, bundle);
    }

    public static boolean isVisible(final int itemId) {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        String.format(
                                Locale.ENGLISH,
                                CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT,
                                itemId
                        ),
                        true);
    }
}

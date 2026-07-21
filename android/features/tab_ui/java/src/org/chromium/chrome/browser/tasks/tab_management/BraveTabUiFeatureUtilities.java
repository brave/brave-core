/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.ui.listmenu.ListItemType;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter;

@NullMarked
public class BraveTabUiFeatureUtilities {
    public static boolean isTabGroupsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
    }

    public static void setTabGroupsEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
    }

    public static boolean isBraveTabGroupsEnabled() {
        return isTabGroupsEnabled() && isOpenLinksInCurrentTabGroupEnabled();
    }

    public static boolean isOpenLinksInCurrentTabGroupEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED,
                        ChromeSharedPreferences.getInstance()
                                .readBoolean(
                                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE,
                                        true));
    }

    public static void setOpenLinksInCurrentTabGroupEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, enabled);
    }

    public static boolean isTabGroupsBarPreferenceEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, true);
    }

    public static void setTabGroupsBarEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, enabled);
    }

    public static boolean isBraveAndroidTabGroupsSettingsFeatureEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS);
    }

    /**
     * Removes the given menu items from a tab overflow/context {@link MVCListAdapter.ModelList} by
     * their {@link ListMenuItemProperties#MENU_ITEM_ID}. Used by Brave tab menu subclasses to strip
     * tab group creation entries when the "Enable tab groups" master switch is off.
     */
    public static void removeMenuItems(MVCListAdapter.ModelList itemList, int... menuIds) {
        for (int i = itemList.size() - 1; i >= 0; i--) {
            MVCListAdapter.ListItem item = itemList.get(i);
            if (item.type != ListItemType.MENU_ITEM) {
                continue;
            }
            int id = item.model.get(ListMenuItemProperties.MENU_ITEM_ID);
            for (int menuId : menuIds) {
                if (id == menuId) {
                    itemList.removeAt(i);
                    break;
                }
            }
        }
    }
}

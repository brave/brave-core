/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ObserverList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.ui.listmenu.ListItemType;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter;

@NullMarked
public class BraveTabUiFeatureUtilities {
    // Created when the first observer is added, not at class initialization: an ObserverList takes
    // the thread it was created on as the only thread it may be used from, and this class is a
    // static utility that gets touched from the instrumentation thread in tests. The observers
    // themselves are only ever added, removed and run on the UI thread.
    private static @Nullable ObserverList<Runnable> sSettingsObservers;

    /**
     * Registers {@code observer} to be run when one of the tab groups switches changes. The
     * switches live in shared preferences, which cannot be observed through {@link
     * org.chromium.base.shared_preferences.SharedPreferencesManager}, so the setters below report
     * the change through here instead.
     */
    public static void addSettingsObserver(Runnable observer) {
        if (sSettingsObservers == null) {
            sSettingsObservers = new ObserverList<>();
        }
        sSettingsObservers.addObserver(observer);
    }

    /** Stops running {@code observer}, which must have been added by the call above. */
    public static void removeSettingsObserver(Runnable observer) {
        if (sSettingsObservers == null) return;
        sSettingsObservers.removeObserver(observer);
    }

    private static void notifySettingsChanged() {
        if (sSettingsObservers == null) return;
        for (Runnable observer : sSettingsObservers) {
            observer.run();
        }
    }

    public static boolean isTabGroupsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
    }

    public static void setTabGroupsEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
        notifySettingsChanged();
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
        notifySettingsChanged();
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

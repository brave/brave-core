/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.ui.listmenu.ListItemType;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

/**
 * Brave's {@link TabSwitcherActionMenuCoordinator}. Removes the "Add to (new) group" entries from
 * the tab switcher button long-press menu when the Brave "Enable tab groups" master switch is off.
 * Instantiated in place of the upstream class via a plaster redirect.
 */
@NullMarked
public class BraveTabSwitcherActionMenuCoordinator extends TabSwitcherActionMenuCoordinator {
    public BraveTabSwitcherActionMenuCoordinator(
            Profile profile,
            MonotonicObservableSupplier<TabModelSelector> tabModelSelectorSupplier) {
        super(profile, tabModelSelectorSupplier);
    }

    @Override
    ModelList buildMenuItems() {
        ModelList itemList = super.buildMenuItems();
        boolean tabGroupsEnabled =
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
        if (!tabGroupsEnabled) {
            for (int i = itemList.size() - 1; i >= 0; i--) {
                ListItem item = itemList.get(i);
                if (item.type != ListItemType.MENU_ITEM) {
                    continue;
                }
                int id = item.model.get(ListMenuItemProperties.MENU_ITEM_ID);
                if (id == R.id.add_tab_to_group_menu_id
                        || id == R.id.add_tab_to_new_group_menu_id) {
                    itemList.removeAt(i);
                }
            }
        }
        return itemList;
    }
}

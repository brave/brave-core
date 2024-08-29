/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.view.View.OnLongClickListener;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

/**
 * The main coordinator for the Tab Switcher Action Menu on the bottom toolbar, responsible for
 * creating the popup menu and building a list of menu items.
 */
public class BottomTabSwitcherActionMenuCoordinator extends TabSwitcherActionMenuCoordinator {
    public BottomTabSwitcherActionMenuCoordinator(
            Profile profile, ObservableSupplier<TabModelSelector> tabModelSelectorSupplier) {
        super(profile, tabModelSelectorSupplier);
    }

    public static OnLongClickListener createOnLongClickListener(
            Callback<Integer> onItemClicked,
            Profile profile,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier) {
        return createOnLongClickListener(
                new BottomTabSwitcherActionMenuCoordinator(profile, tabModelSelectorSupplier),
                profile,
                onItemClicked);
    }

    @Override
    ModelList buildMenuItems() {
        ModelList itemList = new ModelList();
        itemList.add(buildListItemByMenuItemType(MenuItemType.NEW_TAB));
        itemList.add(buildListItemByMenuItemType(MenuItemType.NEW_INCOGNITO_TAB));
        itemList.add(buildListItemByMenuItemType(MenuItemType.DIVIDER));
        itemList.add(buildListItemByMenuItemType(MenuItemType.CLOSE_TAB));
        return itemList;
    }
}

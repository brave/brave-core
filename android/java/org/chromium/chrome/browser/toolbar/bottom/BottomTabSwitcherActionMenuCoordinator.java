// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

import android.view.View;
import android.view.View.OnLongClickListener;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.toolbar.top.TabSwitcherActionMenuCoordinator;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.widget.RectProvider;
import org.chromium.ui.widget.ViewRectProvider;

/**
 * The main coordinator for the Tab Switcher Action Menu on the bottom toolbar,
 * responsible for creating the popup menu and building a list of menu items.
 */
public class BottomTabSwitcherActionMenuCoordinator extends TabSwitcherActionMenuCoordinator {
    public static OnLongClickListener createOnLongClickListener(Callback<Integer> onItemClicked) {
        return createOnLongClickListener(
                new BottomTabSwitcherActionMenuCoordinator(), onItemClicked);
    }

    @Override
    public ModelList buildMenuItems() {
        ModelList itemList = new ModelList();
        itemList.add(buildListItemByMenuItemType(MenuItemType.NEW_TAB));
        itemList.add(buildListItemByMenuItemType(MenuItemType.NEW_INCOGNITO_TAB));
        itemList.add(buildListItemByMenuItemType(MenuItemType.DIVIDER));
        itemList.add(buildListItemByMenuItemType(MenuItemType.CLOSE_TAB));
        return itemList;
    }

    @Override
    protected RectProvider getRectProvider(View anchorView) {
        ViewRectProvider rectProvider = new ViewRectProvider(anchorView);
        rectProvider.setIncludePadding(true);

        // space between the icon and the border of the wrapper
        int toolbarHeight = anchorView.getHeight();
        int iconHeight =
                anchorView.getResources().getDimensionPixelSize(R.dimen.toolbar_icon_height);
        int padding = (toolbarHeight - iconHeight) / 2;
        rectProvider.setInsetPx(0, padding, padding / 2, 0);

        return rectProvider;
    }
}

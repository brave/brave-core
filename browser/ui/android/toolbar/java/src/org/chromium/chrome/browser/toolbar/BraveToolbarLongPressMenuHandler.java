/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import android.content.Context;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.widget.ViewRectProvider;
import org.chromium.url.GURL;

import java.util.function.BooleanSupplier;
import java.util.function.Supplier;

/** Brave's extension for the handler for the toolbar long press menu. */
@NullMarked
public class BraveToolbarLongPressMenuHandler extends ToolbarLongPressMenuHandler {
    public BraveToolbarLongPressMenuHandler(
            Context context,
            ObservableSupplier<Profile> profileSupplier,
            boolean isCustomTab,
            BooleanSupplier suppressLongPressSupplier,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            WindowAndroid windowAndroid,
            Supplier<@Nullable GURL> urlSupplier,
            Supplier<ViewRectProvider> urlBarViewRectProviderSupplier) {
        super(
                context,
                profileSupplier,
                isCustomTab,
                suppressLongPressSupplier,
                lifecycleDispatcher,
                windowAndroid,
                urlSupplier,
                urlBarViewRectProviderSupplier);
    }

    @Override
    ModelList buildMenuItems(boolean onTop) {
        ModelList itemList = super.buildMenuItems(onTop);

        // Remove the "Copy link" item from menu.
        for (int i = 0; i < itemList.size(); i++) {
            int itemID = itemList.get(i).model.get(ListMenuItemProperties.MENU_ITEM_ID);
            if (itemID == MenuItemType.COPY_LINK) {
                itemList.removeAt(i);
                break;
            }
        }
        return itemList;
    }
}

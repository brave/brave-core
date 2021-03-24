/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.appmenu;

import android.content.Context;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.feed.webfeed.WebFeedBridge;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.ui.modaldialog.ModalDialogManager;

public class BraveAppMenuPropertiesDelegateImpl extends AppMenuPropertiesDelegateImpl {
    public BraveAppMenuPropertiesDelegateImpl(Context context,
            ActivityTabProvider activityTabProvider,
            MultiWindowModeStateDispatcher multiWindowModeStateDispatcher,
            TabModelSelector tabModelSelector, ToolbarManager toolbarManager, View decorView,
            @Nullable OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier,
            ModalDialogManager modalDialogManager, WebFeedBridge webFeedBridge) {
        super(context, activityTabProvider, multiWindowModeStateDispatcher, tabModelSelector,
                toolbarManager, decorView, overviewModeBehaviorSupplier, bookmarkBridgeSupplier,
                modalDialogManager, webFeedBridge);
    }

    @Override
    public void prepareMenu(Menu menu, AppMenuHandler handler) {
        super.prepareMenu(menu, handler);

        maybeReplaceIcons(menu);
    }

    private void maybeReplaceIcons(Menu menu) {
        if (shouldShowIconBeforeItem()) {
            for (int i = 0; i < menu.size(); ++i) {
                MenuItem item = menu.getItem(i);
                if (item.hasSubMenu()) {
                    maybeReplaceIcons(item.getSubMenu());
                } else if (item.getItemId() == R.id.new_incognito_tab_menu_id) {
                    item.setIcon(AppCompatResources.getDrawable(
                            mContext, R.drawable.brave_menu_new_private_tab));
                } else if (item.getItemId() == R.id.all_bookmarks_menu_id) {
                    item.setIcon(AppCompatResources.getDrawable(
                            mContext, R.drawable.brave_menu_bookmarks));
                } else if (item.getItemId() == R.id.recent_tabs_menu_id) {
                    item.setIcon(AppCompatResources.getDrawable(
                            mContext, R.drawable.brave_menu_recent_tabs));
                } else if (item.getItemId() == R.id.open_history_menu_id) {
                    item.setIcon(AppCompatResources.getDrawable(
                            mContext, R.drawable.brave_menu_history));
                } else if (item.getItemId() == R.id.downloads_menu_id) {
                    item.setIcon(AppCompatResources.getDrawable(
                            mContext, R.drawable.brave_menu_downloads));
                } else if (item.getItemId() == R.id.preferences_id) {
                    item.setIcon(AppCompatResources.getDrawable(
                            mContext, R.drawable.brave_menu_settings));
                }
            }
        }
    }
}

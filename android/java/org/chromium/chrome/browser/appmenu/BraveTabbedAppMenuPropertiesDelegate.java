/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

import android.content.Context;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;

import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.notifications.BraveSetDefaultBrowserNotificationService;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabbed_mode.TabbedAppMenuPropertiesDelegate;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;

public class BraveTabbedAppMenuPropertiesDelegate extends TabbedAppMenuPropertiesDelegate {
    private Menu mMenu;

    public BraveTabbedAppMenuPropertiesDelegate(Context context,
            ActivityTabProvider activityTabProvider,
            MultiWindowModeStateDispatcher multiWindowModeStateDispatcher,
            TabModelSelector tabModelSelector, ToolbarManager toolbarManager, View decorView,
            AppMenuDelegate appMenuDelegate,
            @Nullable ObservableSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier) {
        super(context, activityTabProvider, multiWindowModeStateDispatcher, tabModelSelector,
                toolbarManager, decorView, appMenuDelegate, overviewModeBehaviorSupplier,
                bookmarkBridgeSupplier);
    }

    @Override
    public void prepareMenu(Menu menu, AppMenuHandler handler) {
        super.prepareMenu(menu, handler);

        mMenu = menu;

        // Brave's items are only visible for page menu.
        // To make logic simple, below three items are added whenever menu gets visible
        // and removed when menu is dismissed.
        if (!shouldShowPageMenu()) return;

        // Brave donesn't show help menu item in app menu.
        menu.findItem(R.id.help_id).setVisible(false).setEnabled(false);

        // Always hide share row menu item in app menu if it's not on tablet.
        if (!mIsTablet) menu.findItem(R.id.share_row_menu_id).setVisible(false);

        menu.add(Menu.NONE, R.id.set_default_browser, 0, R.string.menu_set_default_browser);
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            menu.add(Menu.NONE, R.id.brave_rewards_id, 0, R.string.menu_brave_rewards);
        }
        menu.add(Menu.NONE, R.id.exit_id, 0, R.string.menu_exit);

        if (BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(mContext)) {
            menu.findItem(R.id.set_default_browser).setVisible(false);
        }

        // Replace info item with share
        MenuItem shareItem = menu.findItem(R.id.info_menu_id);
        if (shareItem != null) {
            shareItem.setTitle(mContext.getString(R.string.share));
            shareItem.setIcon(AppCompatResources.getDrawable(mContext, R.drawable.share_icon));
        }

        // By this we forcibly initialize mBookmarkBridge
        MenuItem bookmarkItem = menu.findItem(R.id.bookmark_this_page_id);
        Tab currentTab = mActivityTabProvider.get();
        if (bookmarkItem != null && currentTab != null) {
            updateBookmarkMenuItem(bookmarkItem, currentTab);
        }
    }

    @Override
    public void onMenuDismissed() {
        super.onMenuDismissed();

        mMenu.removeItem(R.id.set_default_browser);
        mMenu.removeItem(R.id.brave_rewards_id);
        mMenu.removeItem(R.id.exit_id);
    }

    @Override
    public void onFooterViewInflated(AppMenuHandler appMenuHandler, View view) {
        // If it's still null, just hide the whole view
        if (mBookmarkBridge == null) {
            if (view != null) {
                view.setVisibility(View.GONE);
            }
            // Normally it should not happen
            assert false;
            return;
        }
        super.onFooterViewInflated(appMenuHandler, view);

        // Hide bookmark button if bottom toolbar is enabled
        ImageButton bookmarkButton = view.findViewById(R.id.bookmark_this_page_id);
        if (bookmarkButton != null && BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            bookmarkButton.setVisibility(View.GONE);
        }

        // Replace info button with share
        ImageButton shareButton = view.findViewById(R.id.info_menu_id);
        if (shareButton != null) {
            shareButton.setImageDrawable(
                    AppCompatResources.getDrawable(mContext, R.drawable.share_icon));
            shareButton.setContentDescription(mContext.getString(R.string.share));
        }
    }
}

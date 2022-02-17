/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

import android.content.Context;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.widget.ImageButton;

import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.app.appmenu.AppMenuIconRowFooter;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.feed.webfeed.WebFeedBridge;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.notifications.BraveSetDefaultBrowserNotificationService;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabbed_mode.TabbedAppMenuPropertiesDelegate;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.vpn.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.ui.modaldialog.ModalDialogManager;

public class BraveTabbedAppMenuPropertiesDelegate extends TabbedAppMenuPropertiesDelegate {
    private Menu mMenu;
    private AppMenuDelegate mAppMenuDelegate;
    private ObservableSupplier<BookmarkBridge> mBookmarkBridgeSupplier;

    public BraveTabbedAppMenuPropertiesDelegate(Context context,
            ActivityTabProvider activityTabProvider,
            MultiWindowModeStateDispatcher multiWindowModeStateDispatcher,
            TabModelSelector tabModelSelector, ToolbarManager toolbarManager, View decorView,
            AppMenuDelegate appMenuDelegate,
            OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            OneshotSupplier<StartSurface> startSurfaceSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier,
            WebFeedSnackbarController.FeedLauncher feedLauncher,
            ModalDialogManager modalDialogManager, SnackbarManager snackbarManager) {
        super(context, activityTabProvider, multiWindowModeStateDispatcher, tabModelSelector,
                toolbarManager, decorView, appMenuDelegate, overviewModeBehaviorSupplier,
                startSurfaceSupplier, bookmarkBridgeSupplier, feedLauncher, modalDialogManager,
                snackbarManager);

        mAppMenuDelegate = appMenuDelegate;
        mBookmarkBridgeSupplier = bookmarkBridgeSupplier;
    }

    @Override
    public void prepareMenu(Menu menu, AppMenuHandler handler) {
        super.prepareMenu(menu, handler);

        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            menu.addSubMenu(Menu.NONE, R.id.request_brave_vpn_row_menu_id, 0, null);
            SubMenu vpnSubMenu = menu.findItem(R.id.request_brave_vpn_row_menu_id).getSubMenu();
            vpnSubMenu.clear();
            MenuItem braveVpnSubMenuItem =
                    vpnSubMenu.add(Menu.NONE, R.id.request_brave_vpn_id, 0, R.string.brave_vpn);
            if (shouldShowIconBeforeItem()) {
                braveVpnSubMenuItem.setIcon(
                        AppCompatResources.getDrawable(mContext, R.drawable.ic_vpn));
            }
            MenuItem braveVpnCheckedSubMenuItem =
                    vpnSubMenu.add(Menu.NONE, R.id.request_brave_vpn_check_id, 0, null);
            braveVpnCheckedSubMenuItem.setCheckable(true);
            braveVpnCheckedSubMenuItem.setChecked(
                    BraveVpnProfileUtils.getInstance().isVPNConnected(mContext));
        }

        mMenu = menu;

        // Brave's items are only visible for page menu.
        // To make logic simple, below three items are added whenever menu gets visible
        // and removed when menu is dismissed.

        if (!shouldShowPageMenu()) return;

        if (isMenuButtonInBottomToolbar()) {
            // Do not show icon row on top when menu itself is on bottom
            menu.findItem(R.id.icon_row_menu_id).setVisible(false).setEnabled(false);
        }

        // Brave donesn't show help menu item in app menu.
        menu.findItem(R.id.help_id).setVisible(false).setEnabled(false);

        // Always hide share row menu item in app menu if it's not on tablet.
        if (!mIsTablet) menu.findItem(R.id.share_row_menu_id).setVisible(false);

        MenuItem setAsDefault =
                menu.add(Menu.NONE, R.id.set_default_browser, 0, R.string.menu_set_default_browser);
        if (shouldShowIconBeforeItem()) {
            setAsDefault.setIcon(
                    AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_set_as_default));
        }

        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            MenuItem rewards =
                    menu.add(Menu.NONE, R.id.brave_rewards_id, 0, R.string.menu_brave_rewards);
            if (shouldShowIconBeforeItem()) {
                rewards.setIcon(
                        AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_rewards));
            }
        }
        MenuItem braveWallet = menu.findItem(R.id.brave_wallet_id);
        if (braveWallet != null) {
            if (ChromeFeatureList.isEnabled(BraveFeatureList.NATIVE_BRAVE_WALLET)) {
                braveWallet.setVisible(true);
                if (shouldShowIconBeforeItem()) {
                    braveWallet.setIcon(
                            AppCompatResources.getDrawable(mContext, R.drawable.ic_crypto_wallets));
                }
            } else {
                braveWallet.setVisible(false);
            }
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
            MenuItem braveNews =
                    menu.add(Menu.NONE, R.id.brave_news_id, 0, R.string.brave_news_title);
            if (shouldShowIconBeforeItem()) {
                braveNews.setIcon(AppCompatResources.getDrawable(mContext, R.drawable.ic_news));
            }
        }
        MenuItem exit = menu.add(Menu.NONE, R.id.exit_id, 0, R.string.menu_exit);
        if (shouldShowIconBeforeItem()) {
            exit.setIcon(AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_exit));
        }

        if (BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(mContext)) {
            menu.findItem(R.id.set_default_browser).setVisible(false);
        }

        // Replace info item with share
        MenuItem shareItem = menu.findItem(R.id.info_menu_id);
        if (shareItem != null) {
            shareItem.setTitle(mContext.getString(R.string.share));
            shareItem.setIcon(AppCompatResources.getDrawable(mContext, R.drawable.share_icon));
        }

        // By this we forcibly initialize BookmarkBridge
        MenuItem bookmarkItem = menu.findItem(R.id.bookmark_this_page_id);
        Tab currentTab = mActivityTabProvider.get();
        if (bookmarkItem != null && currentTab != null) {
            updateBookmarkMenuItemShortcut(bookmarkItem, currentTab, /*fromCCT=*/false);
        }
    }

    @Override
    public void onMenuDismissed() {
        super.onMenuDismissed();

        mMenu.removeItem(R.id.set_default_browser);
        mMenu.removeItem(R.id.brave_rewards_id);
        mMenu.removeItem(R.id.brave_wallet_id);
        mMenu.removeItem(R.id.exit_id);
        if (BraveVpnUtils.isBraveVpnFeatureEnable())
            mMenu.removeItem(R.id.request_brave_vpn_row_menu_id);
    }

    @Override
    public void onFooterViewInflated(AppMenuHandler appMenuHandler, View view) {
        // If it's still null, just hide the whole view
        if (mBookmarkBridgeSupplier.get() == null) {
            if (view != null) {
                view.setVisibility(View.GONE);
            }
            // Normally it should not happen
            assert false;
            return;
        }
        super.onFooterViewInflated(appMenuHandler, view);

        if (view instanceof AppMenuIconRowFooter) {
            ((AppMenuIconRowFooter) view)
                    .initialize(appMenuHandler, mBookmarkBridgeSupplier.get(),
                            mActivityTabProvider.get(), mAppMenuDelegate);
        }

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

    @Override
    public boolean shouldShowHeader(int maxMenuHeight) {
        if (isMenuButtonInBottomToolbar()) return false;
        return super.shouldShowHeader(maxMenuHeight);
    }

    @Override
    public boolean shouldShowFooter(int maxMenuHeight) {
        if (isMenuButtonInBottomToolbar()) return true;
        return super.shouldShowFooter(maxMenuHeight);
    }

    @Override
    public int getFooterResourceId() {
        if (isMenuButtonInBottomToolbar()) {
            return shouldShowPageMenu() ? R.layout.icon_row_menu_footer : 0;
        }
        return super.getFooterResourceId();
    }

    private boolean isMenuButtonInBottomToolbar() {
        return BraveMenuButtonCoordinator.isMenuFromBottom();
    }
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageButton;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.drawable.DrawableCompat;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.brave_vpn.mojom.BraveVpnConstants;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.app.appmenu.AppMenuIconRowFooter;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowUtils;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.readaloud.ReadAloudController;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.speedreader.BraveSpeedReaderUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler.AppMenuItemType;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Arrays;
import java.util.List;

/** Brave's extension for TabbedAppMenuPropertiesDelegate */
@NullMarked
public class BraveTabbedAppMenuPropertiesDelegate extends TabbedAppMenuPropertiesDelegate {
    private final AppMenuDelegate mAppMenuDelegate;
    private final ObservableSupplier<BookmarkModel> mBookmarkModelSupplier;

    public BraveTabbedAppMenuPropertiesDelegate(
            Context context,
            ActivityTabProvider activityTabProvider,
            MultiWindowModeStateDispatcher multiWindowModeStateDispatcher,
            TabModelSelector tabModelSelector,
            ToolbarManager toolbarManager,
            View decorView,
            AppMenuDelegate appMenuDelegate,
            OneshotSupplier<LayoutStateProvider> layoutStateProvider,
            ObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            WebFeedSnackbarController.FeedLauncher feedLauncher,
            ModalDialogManager modalDialogManager,
            SnackbarManager snackbarManager,
            @NonNull
                    OneshotSupplier<IncognitoReauthController>
                            incognitoReauthControllerOneshotSupplier,
            Supplier<ReadAloudController> readAloudControllerSupplier) {
        super(
                context,
                activityTabProvider,
                multiWindowModeStateDispatcher,
                tabModelSelector,
                toolbarManager,
                decorView,
                appMenuDelegate,
                layoutStateProvider,
                bookmarkModelSupplier,
                feedLauncher,
                modalDialogManager,
                snackbarManager,
                incognitoReauthControllerOneshotSupplier,
                readAloudControllerSupplier);

        mAppMenuDelegate = appMenuDelegate;
        mBookmarkModelSupplier = bookmarkModelSupplier;
    }

    @Override
    public void onFooterViewInflated(AppMenuHandler appMenuHandler, View view) {
        // If it's still null, just hide the whole view
        if (mBookmarkModelSupplier.get() == null) {
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
                    .initialize(
                            appMenuHandler,
                            mBookmarkModelSupplier.get(),
                            mActivityTabProvider.get(),
                            mAppMenuDelegate);
        }

        // Hide bookmark button if bottom toolbar is enabled and address bar is on top.
        ImageButton bookmarkButton = view.findViewById(R.id.bookmark_this_page_id);
        if (bookmarkButton != null
                && BottomToolbarConfiguration.isBraveBottomControlsEnabled()
                && BottomToolbarConfiguration.isToolbarTopAnchored()) {
            bookmarkButton.setVisibility(View.GONE);
        }

        boolean showForwardButton = BottomToolbarConfiguration.isToolbarTopAnchored();
        ImageButton forwardButton = view.findViewById(R.id.forward_menu_id);
        if (forwardButton != null) {
            showForwardButton = showForwardButton || forwardButton.isEnabled();
            forwardButton.setVisibility(showForwardButton ? View.VISIBLE : View.GONE);
        }

        ImageButton shareButton = view.findViewById(R.id.share_menu_id);
        boolean showShareButton =
                BottomToolbarConfiguration.isToolbarTopAnchored() || !showForwardButton;
        if (shareButton != null) {
            Tab currentTab = mActivityTabProvider.get();
            if (currentTab != null && UrlUtilities.isNtpUrl(currentTab.getUrl().getSpec())) {
                shareButton.setEnabled(false);
            }
            shareButton.setVisibility(showShareButton ? View.VISIBLE : View.GONE);
        }

        ImageButton homeButton = view.findViewById(R.id.home_menu_id);
        if (homeButton != null && HomepageManager.getInstance() != null) {
            homeButton.setVisibility(
                    BottomToolbarConfiguration.isToolbarBottomAnchored()
                                    && HomepageManager.getInstance().isHomepageEnabled()
                            ? View.VISIBLE
                            : View.GONE);
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

    private void maybeReplaceIcons(MVCListAdapter.ModelList modelList) {
        if (!shouldShowIconBeforeItem()) return;

        for (int i = 0; i < modelList.size(); ++i) {
            MVCListAdapter.ListItem item = modelList.get(i);
            Integer itemId = item.model.get(AppMenuItemProperties.MENU_ITEM_ID);
            if (itemId == null) continue;

            if (itemId == R.id.new_tab_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mContext, R.drawable.ic_new_tab_page));
            } else if (itemId == R.id.new_incognito_tab_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mContext, R.drawable.brave_menu_new_private_tab));
            } else if (itemId == R.id.all_bookmarks_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_bookmarks));
            } else if (itemId == R.id.recent_tabs_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mContext, R.drawable.brave_menu_recent_tabs));
            } else if (itemId == R.id.open_history_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_history));
            } else if (itemId == R.id.downloads_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_downloads));
            } else if (itemId == R.id.preferences_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_settings));
            } else if (itemId == R.id.download_page_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mContext, R.drawable.ic_download));
            }
        }
    }

    @Override
    public boolean shouldShowNewWindow() {
        return BraveMultiWindowUtils.shouldEnableMultiWindows() && super.shouldShowNewWindow();
    }

    @Override
    protected boolean shouldShowMoveToOtherWindow() {
        return BraveMultiWindowUtils.shouldEnableMultiWindows()
                && super.shouldShowMoveToOtherWindow();
    }

    @Override
    public MVCListAdapter.ModelList buildMenuModelList(AppMenuHandler handler) {
        MVCListAdapter.ModelList modelList = super.buildMenuModelList(handler);

        int menuGroup = getMenuGroup();
        if (menuGroup == MenuGroup.PAGE_MENU) {
            populateBravePageModeMenu(modelList, handler);
        }

        return modelList;
    }

    private void populateBravePageModeMenu(
            MVCListAdapter.ModelList modelList, AppMenuHandler handler) {
        // Remove items that we don't use in Brave.
        maybeRemoveMenuItems(
                modelList,
                R.id.quick_delete_menu_id,
                R.id.quick_delete_divider_line_id,
                R.id.help_id);
        if (!mIsTablet) {
            maybeRemoveMenuItems(modelList, R.id.share_menu_id);
        }

        // Apply Brave icons.
        maybeReplaceIcons(modelList);

        // Add Brave specific items.
        if (ChromeFeatureList.isEnabled(BraveFeatureList.NATIVE_BRAVE_WALLET)) {
            addMenuItemAfter(
                    modelList, buildBraveWalletItem(), Arrays.asList(R.id.all_bookmarks_menu_id));
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)) {
            addMenuItemAfter(
                    modelList,
                    buildBravePlaylistItem(),
                    Arrays.asList(R.id.brave_wallet_id, R.id.all_bookmarks_menu_id));
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)
                && !ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ADD_TO_PLAYLIST_BUTTON, true)
                && BraveToolbarLayoutImpl.mShouldShowPlaylistMenu) {
            addMenuItemAfter(
                    modelList,
                    buildBraveAddToPlaylistItem(),
                    Arrays.asList(
                            R.id.brave_playlist_id,
                            R.id.brave_wallet_id,
                            R.id.all_bookmarks_menu_id));
        }
        if (BraveLeoPrefUtils.isLeoEnabled()) {
            Tab tab = mActivityTabProvider.get();
            if (tab != null && !tab.isIncognito()) {
                addMenuItemAfter(
                        modelList,
                        buildBraveLeoItem(),
                        Arrays.asList(
                                R.id.add_to_playlist_id,
                                R.id.brave_playlist_id,
                                R.id.brave_wallet_id,
                                R.id.all_bookmarks_menu_id));
            }
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SPEEDREADER)
                && UserPrefs.get(assumeNonNull(mTabModelSelector.getCurrentModel().getProfile()))
                        .getBoolean(BravePref.SPEEDREADER_PREF_ENABLED)) {
            final Tab currentTab = mActivityTabProvider.get();
            if (currentTab != null && BraveSpeedReaderUtils.tabSupportsDistillation(currentTab)) {
                addMenuItemAfter(
                        modelList, buildBraveSpeedreaderItem(), Arrays.asList(R.id.page_zoom_id));
            }
        }
        if (!BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(mContext)) {
            modelList.add(buildSetDefaultBrowserItem());
        }
        if (BraveVpnUtils.isVpnFeatureSupported(mContext)) {
            modelList.add(buildBraveVpnItem());
            if (BraveVpnPrefUtils.isSubscriptionPurchase()
                    && !TextUtils.isEmpty(BraveVpnPrefUtils.getRegionIsoCode())) {
                modelList.add(buildBraveVpnLocationIconItem());
            }
        }
        BraveRewardsNativeWorker braveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (braveRewardsNativeWorker != null && braveRewardsNativeWorker.isSupported()) {
            modelList.add(buildBraveRewardsItem());
        }
        modelList.add(buildBraveNewsItem());
        modelList.add(buildExitItem());
    }

    private void maybeRemoveMenuItems(MVCListAdapter.ModelList modelList, int... itemIds) {
        for (int itemId : itemIds) {
            for (int i = 0; i < modelList.size(); ++i) {
                MVCListAdapter.ListItem item = modelList.get(i);
                if (item.model.get(AppMenuItemProperties.MENU_ITEM_ID) == itemId) {
                    modelList.removeAt(i);
                    break;
                }
            }
        }
    }

    /**
     * Adds a new menu item after the last item with an ID in the previousItemIds list.
     *
     * @param modelList The model list to which the item will be added.
     * @param itemToAdd The item to add to the model list.
     * @param previousItemIds A list of IDs of items that should precede the new item.
     */
    private void addMenuItemAfter(
            MVCListAdapter.ModelList modelList,
            MVCListAdapter.ListItem itemToAdd,
            List<Integer> previousItemIds) {
        for (int i = modelList.size() - 1; i >= 0; i--) {
            MVCListAdapter.ListItem item = modelList.get(i);
            if (previousItemIds.contains(item.model.get(AppMenuItemProperties.MENU_ITEM_ID))) {
                modelList.add(i + 1, itemToAdd);
                return;
            }
        }
        // If the previous item is not found, just add it to the end.
        modelList.add(itemToAdd);
    }

    boolean shouldShowIconRow() {
        if (isMenuButtonInBottomToolbar()) {
            return false;
        }

        return super.shouldShowIconRow();
    }

    private MVCListAdapter.ListItem buildSetDefaultBrowserItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.set_default_browser,
                        R.string.menu_set_default_browser,
                        shouldShowIconBeforeItem() ? R.drawable.brave_menu_set_as_default : 0));
    }

    private MVCListAdapter.ListItem buildExitItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.exit_id,
                        R.string.menu_exit,
                        shouldShowIconBeforeItem() ? R.drawable.brave_menu_exit : 0));
    }

    private MVCListAdapter.ListItem buildBraveRewardsItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_rewards_id,
                        R.string.menu_brave_rewards,
                        shouldShowIconBeforeItem() ? R.drawable.brave_menu_rewards : 0));
    }

    private MVCListAdapter.ListItem buildBraveWalletItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_wallet_id,
                        R.string.menu_brave_wallet,
                        shouldShowIconBeforeItem() ? R.drawable.ic_crypto_wallets : 0));
    }

    private MVCListAdapter.ListItem buildBravePlaylistItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_playlist_id,
                        R.string.brave_playlist,
                        shouldShowIconBeforeItem() ? R.drawable.ic_open_playlist : 0));
    }

    private MVCListAdapter.ListItem buildBraveAddToPlaylistItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.add_to_playlist_id,
                        R.string.playlist_add_to_playlist,
                        shouldShowIconBeforeItem() ? R.drawable.ic_baseline_add_24 : 0));
    }

    private MVCListAdapter.ListItem buildBraveNewsItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_news_id,
                        R.string.brave_news_title,
                        shouldShowIconBeforeItem() ? R.drawable.ic_news : 0));
    }

    private MVCListAdapter.ListItem buildBraveSpeedreaderItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_speedreader_id,
                        R.string.brave_speedreader_title,
                        shouldShowIconBeforeItem() ? R.drawable.ic_readermode : 0));
    }

    private MVCListAdapter.ListItem buildBraveLeoItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_leo_id,
                        R.string.menu_brave_leo,
                        shouldShowIconBeforeItem() ? R.drawable.ic_brave_ai : 0));
    }

    private MVCListAdapter.ListItem buildBraveVpnItem() {
        return new MVCListAdapter.ListItem(
                AppMenuItemType.TITLE_BUTTON,
                buildModelForMenuItemWithCheckbox(
                        R.id.request_brave_vpn_id,
                        R.string.brave_vpn,
                        shouldShowIconBeforeItem() ? R.drawable.ic_vpn : 0,
                        R.id.request_brave_vpn_check_id,
                        BraveVpnProfileUtils.getInstance().isBraveVPNConnected(mContext)));
    }

    private MVCListAdapter.ListItem buildBraveVpnLocationIconItem() {
        String serverLocation = " %s  %s - %s";
        String regionName =
                BraveVpnPrefUtils.getRegionPrecision()
                                .equals(BraveVpnConstants.REGION_PRECISION_COUNTRY)
                        ? mContext.getString(R.string.optimal_text)
                        : BraveVpnPrefUtils.getRegionNamePretty();
        Drawable secondaryActionIcon =
                AppCompatResources.getDrawable(mContext, R.drawable.ic_chevron_right);
        secondaryActionIcon = DrawableCompat.wrap(secondaryActionIcon);
        DrawableCompat.setTint(
                secondaryActionIcon,
                ContextCompat.getColor(mContext, R.color.vpn_timer_icon_color));
        PropertyModel model =
                buildModelForMenuItemWithSecondaryButton(
                        R.id.request_vpn_location_id,
                        R.string.change_location,
                        0 /* iconResId */,
                        R.id.request_vpn_location_icon_id,
                        "" /* secondaryActionTitle */,
                        secondaryActionIcon);
        model.set(
                AppMenuItemProperties.TITLE,
                String.format(
                        serverLocation,
                        BraveVpnUtils.countryCodeToEmoji(BraveVpnPrefUtils.getRegionIsoCode()),
                        BraveVpnPrefUtils.getRegionIsoCode(),
                        regionName));
        return new MVCListAdapter.ListItem(AppMenuItemType.TITLE_BUTTON, model);
    }

    @Override
    protected PropertyModel buildPageInfoModel(@Nullable Tab currentTab) {
        // Instead of the info button, we show the share button in Brave.
        PropertyModel shareButton =
                buildModelForIcon(
                        R.id.info_menu_id, R.string.share, R.string.share, R.drawable.share_icon);
        shareButton.set(
                AppMenuItemProperties.ENABLED,
                (currentTab != null && !UrlUtilities.isNtpUrl(currentTab.getUrl().getSpec())));
        return shareButton;
    }
}

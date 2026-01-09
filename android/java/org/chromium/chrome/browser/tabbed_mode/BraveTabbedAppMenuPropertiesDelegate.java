/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.drawable.DrawableCompat;

import com.google.android.material.button.MaterialButton;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveUrlConstants;
import org.chromium.base.Callback;
import org.chromium.base.DeviceInfo;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.brave.browser.customize_menu.CustomizeBraveMenu;
import org.chromium.brave_vpn.mojom.BraveVpnConstants;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.app.appmenu.AppMenuIconRowFooter;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.brave_origin.BraveOriginSubscriptionPrefs;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowUtils;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.policy.BravePolicyConstants;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.readaloud.ReadAloudController;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tinker_tank.TinkerTankDelegate;
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
import org.chromium.components.browser_ui.accessibility.PageZoomManager;
import org.chromium.components.dom_distiller.core.DomDistillerFeatures;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.webapps.WebappsUtils;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Supplier;

/** Brave's extension for TabbedAppMenuPropertiesDelegate */
@NullMarked
public class BraveTabbedAppMenuPropertiesDelegate extends TabbedAppMenuPropertiesDelegate {
    private final AppMenuDelegate mBraveAppMenuDelegate;
    private final ObservableSupplier<BookmarkModel> mBookmarkModelSupplier;
    private boolean mJunitIsTesting;
    private final Context mBraveContext;

    /**
     * Represents a menu item that can be controlled by policy.
     *
     * <p>To add a new policy-controlled menu item, add a new entry to {@link
     * #getPolicyControlledMenuItems()} with: - menuItemId: The resource ID of the menu item (e.g.,
     * R.id.brave_news_id) - policyKey: The policy constant (e.g.,
     * BravePolicyConstants.BRAVE_NEWS_DISABLED) - itemBuilder: Method reference to build the menu
     * item (e.g., this::buildBraveNewsItem) - isSupportedChecker: Lambda to check if feature is
     * supported (e.g., () -> true) - insertBeforeItemId: Menu item ID to insert before (e.g.,
     * R.id.brave_customize_id)
     *
     * <p>The item will automatically be hidden/shown in the app menu based on policy ({@link
     * #updateMenuItemsBasedOnPolicy}) and hidden/shown in the settings screen menu based on policy
     * ({@link #buildMainMenuModelListAsync}).
     */
    private static class PolicyControlledMenuItem {
        final int mMenuItemId;
        final String mPolicyKey;
        final Supplier<MVCListAdapter.ListItem> mItemBuilder;
        final Supplier<Boolean> mIsSupportedChecker;
        final int mInsertBeforeItemId;

        PolicyControlledMenuItem(
                int menuItemId,
                String policyKey,
                Supplier<MVCListAdapter.ListItem> itemBuilder,
                Supplier<Boolean> isSupportedChecker,
                int insertBeforeItemId) {
            mMenuItemId = menuItemId;
            mPolicyKey = policyKey;
            mItemBuilder = itemBuilder;
            mIsSupportedChecker = isSupportedChecker;
            mInsertBeforeItemId = insertBeforeItemId;
        }
    }

    /**
     * Configuration list of all policy-controlled menu items. To add a new feature, simply add an
     * entry here.
     */
    private List<PolicyControlledMenuItem> getPolicyControlledMenuItems() {
        return Arrays.asList(
                new PolicyControlledMenuItem(
                        R.id.brave_rewards_id,
                        BravePolicyConstants.BRAVE_REWARDS_DISABLED,
                        this::buildBraveRewardsItem,
                        () -> {
                            BraveRewardsNativeWorker worker =
                                    BraveRewardsNativeWorker.getInstance();
                            return worker != null && worker.isSupported();
                        },
                        R.id.brave_news_id));
    }

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
            Supplier<ReadAloudController> readAloudControllerSupplier,
            PageZoomManager pageZoomManager) {
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
                readAloudControllerSupplier,
                pageZoomManager);

        mBraveAppMenuDelegate = appMenuDelegate;
        mBookmarkModelSupplier = bookmarkModelSupplier;
        mBraveContext = context;
    }

    /**
     * Helper method to get the current profile from the activity tab provider. Used for policy
     * checks that require a profile.
     *
     * @return The profile, or null if not available
     */
    @Nullable
    private Profile getCurrentProfile() {
        Tab currentTab = mActivityTabProvider.get();
        if (currentTab == null) {
            return null;
        }
        return Profile.fromWebContents(currentTab.getWebContents());
    }

    /**
     * Checks if a menu item is present in the menu list.
     *
     * @param modelList The menu model list to search
     * @param menuItemId The menu item ID to look for
     * @return true if the item is present, false otherwise
     */
    private boolean isMenuItemPresent(MVCListAdapter.ModelList modelList, int menuItemId) {
        for (int i = 0; i < modelList.size(); i++) {
            MVCListAdapter.ListItem item = modelList.get(i);
            if (item.model.get(AppMenuItemProperties.MENU_ITEM_ID) == menuItemId) {
                return true;
            }
        }
        return false;
    }

    /**
     * Removes a menu item by ID from the menu list.
     *
     * @param modelList The menu model list to modify
     * @param menuItemId The menu item ID to remove
     */
    private void removeMenuItemById(MVCListAdapter.ModelList modelList, int menuItemId) {
        for (int i = 0; i < modelList.size(); i++) {
            MVCListAdapter.ListItem item = modelList.get(i);
            if (item.model.get(AppMenuItemProperties.MENU_ITEM_ID) == menuItemId) {
                modelList.removeAt(i);
                break;
            }
        }
    }

    /**
     * Inserts a menu item before a specified item in the menu list.
     *
     * @param modelList The menu model list to modify
     * @param itemToInsert The menu item to insert
     * @param beforeItemId The menu item ID to insert before
     */
    private void insertMenuItemBefore(
            MVCListAdapter.ModelList modelList,
            MVCListAdapter.ListItem itemToInsert,
            int beforeItemId) {
        int insertIndex = modelList.size() - 1; // Default to end if beforeItemId not found
        for (int i = 0; i < modelList.size(); i++) {
            MVCListAdapter.ListItem item = modelList.get(i);
            if (item.model.get(AppMenuItemProperties.MENU_ITEM_ID) == beforeItemId) {
                insertIndex = i;
                break;
            }
        }
        modelList.add(insertIndex, itemToInsert);
    }

    /**
     * Updates menu items based on policy checks for all configured policy-controlled items. This
     * method checks policies asynchronously and adds/removes menu items accordingly.
     *
     * @param modelList The menu model list to update
     */
    private void updateMenuItemsBasedOnPolicy(MVCListAdapter.ModelList modelList) {
        Profile profile = getCurrentProfile();
        for (PolicyControlledMenuItem item : getPolicyControlledMenuItems()) {
            BraveOriginSubscriptionPrefs.checkPolicyAsync(
                    profile,
                    item.mPolicyKey,
                    (isDisabled) -> {
                        if (isDisabled) {
                            // Policy disables the feature - remove item if present
                            removeMenuItemById(modelList, item.mMenuItemId);
                        } else {
                            // Policy allows the feature - add item if not present and supported
                            if (!isMenuItemPresent(modelList, item.mMenuItemId)) {
                                if (item.mIsSupportedChecker.get()) {
                                    insertMenuItemBefore(
                                            modelList,
                                            item.mItemBuilder.get(),
                                            item.mInsertBeforeItemId);
                                }
                            }
                        }
                    });
        }
    }

    private void onFooterViewInflated(AppMenuHandler appMenuHandler, View view) {
        // If it's still null, just hide the whole view
        if (mBookmarkModelSupplier.get() == null) {
            if (view != null) {
                view.setVisibility(View.GONE);
            }
            // Normally it should not happen
            assert false;
            return;
        }

        if (view instanceof AppMenuIconRowFooter) {
            ((AppMenuIconRowFooter) view)
                    .initialize(
                            appMenuHandler,
                            mBookmarkModelSupplier.get(),
                            mActivityTabProvider.get(),
                            mBraveAppMenuDelegate);
        }

        // Hide bookmark button if bottom toolbar is enabled and address bar is on top.
        View bookmarkWrapper = view.findViewById(R.id.button_wrapper_bookmark);
        MaterialButton bookmarkButton = view.findViewById(R.id.bookmark_this_page_id);
        if (bookmarkButton != null
                && BottomToolbarConfiguration.isBraveBottomControlsEnabled()
                && BottomToolbarConfiguration.isToolbarTopAnchored()) {
            if (bookmarkWrapper != null) bookmarkWrapper.setVisibility(View.GONE);
        }

        boolean showForwardButton = BottomToolbarConfiguration.isToolbarTopAnchored();
        View forwardWrapper = view.findViewById(R.id.button_wrapper_forward);
        MaterialButton forwardButton = view.findViewById(R.id.forward_menu_id);
        if (forwardButton != null) {
            showForwardButton = showForwardButton || forwardButton.isEnabled();
            if (forwardWrapper != null) {
                forwardWrapper.setVisibility(showForwardButton ? View.VISIBLE : View.GONE);
            }
        }

        View shareWrapper = view.findViewById(R.id.button_wrapper_share);
        MaterialButton shareButton = view.findViewById(R.id.share_menu_id);
        boolean showShareButton =
                BottomToolbarConfiguration.isToolbarTopAnchored() || !showForwardButton;
        if (shareButton != null) {
            Tab currentTab = mActivityTabProvider.get();
            if (currentTab != null && UrlUtilities.isNtpUrl(currentTab.getUrl().getSpec())) {
                shareButton.setEnabled(false);
            }
            if (shareWrapper != null) {
                shareWrapper.setVisibility(showShareButton ? View.VISIBLE : View.GONE);
            }
        }

        View homeWrapper = view.findViewById(R.id.button_wrapper_home);
        MaterialButton homeButton = view.findViewById(R.id.home_menu_id);
        if (homeButton != null && HomepageManager.getInstance() != null) {
            boolean showHome =
                    BottomToolbarConfiguration.isToolbarBottomAnchored()
                            && HomepageManager.getInstance().isHomepageEnabled();
            if (homeWrapper != null) {
                homeWrapper.setVisibility(showHome ? View.VISIBLE : View.GONE);
            }
        }
    }

    @Override
    public @Nullable View buildFooterView(AppMenuHandler appMenuHandler) {
        if (isMenuButtonInBottomToolbar() && shouldShowPageMenu()) {
            View footer =
                    LayoutInflater.from(mBraveContext).inflate(R.layout.icon_row_menu_footer, null);

            this.onFooterViewInflated(appMenuHandler, footer);

            return footer;
        }
        return null;
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
                        AppCompatResources.getDrawable(mBraveContext, R.drawable.ic_new_tab_page));
            } else if (itemId == R.id.new_incognito_tab_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mBraveContext, R.drawable.brave_menu_new_private_tab));
            } else if (itemId == R.id.new_tab_group_menu_id
                    || itemId == R.id.add_to_group_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mBraveContext, R.drawable.browser_group));
            } else if (itemId == R.id.all_bookmarks_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mBraveContext, R.drawable.brave_menu_bookmarks));
            } else if (itemId == R.id.recent_tabs_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mBraveContext, R.drawable.brave_menu_recent_tabs));
            } else if (itemId == R.id.open_history_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mBraveContext, R.drawable.brave_menu_history));
            } else if (itemId == R.id.downloads_menu_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mBraveContext, R.drawable.brave_menu_downloads));
            } else if (itemId == R.id.preferences_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(
                                mBraveContext, R.drawable.brave_menu_settings));
            } else if (itemId == R.id.download_page_id) {
                item.model.set(
                        AppMenuItemProperties.ICON,
                        AppCompatResources.getDrawable(mBraveContext, R.drawable.ic_download));
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

    /**
     * Builds the complete list of main menu items for the Customize menu settings screen
     * asynchronously, ensuring policy values are checked before building.
     *
     * <p>This method checks all policy-controlled menu items asynchronously, then builds the menu
     * list with the correct policy state. Use this method instead of {@link
     * #buildMainMenuModelList()} when you need to ensure policy values are up-to-date.
     *
     * @param callback Called with the built menu model list once all policy checks complete
     */
    public void buildMainMenuModelListAsync(Callback<MVCListAdapter.ModelList> callback) {
        // Always check policy fresh (no caching) before building menu
        // Check all policy-controlled items, then build menu with policy states
        Profile profile = getCurrentProfile();
        List<PolicyControlledMenuItem> policyItems = getPolicyControlledMenuItems();

        if (policyItems.isEmpty()) {
            // No policy-controlled items, build menu without policy checks
            MVCListAdapter.ModelList menuList = buildMainMenuModelList(new HashMap<>());
            callback.onResult(menuList);
            return;
        }

        // Map to store policy states: menuItemId -> isDisabled
        Map<Integer, Boolean> policyStates = new HashMap<>();
        final int[] pendingChecks = {
            policyItems.size()
        }; // Use array to allow modification in lambda

        for (PolicyControlledMenuItem item : policyItems) {
            BraveOriginSubscriptionPrefs.checkPolicyAsync(
                    profile,
                    item.mPolicyKey,
                    (isDisabled) -> {
                        policyStates.put(item.mMenuItemId, isDisabled);
                        pendingChecks[0]--;

                        // When all policy checks are complete, build the menu
                        if (pendingChecks[0] == 0) {
                            MVCListAdapter.ModelList menuList =
                                    buildMainMenuModelList(policyStates);
                            callback.onResult(menuList);
                        }
                    });
        }
    }

    /**
     * Builds the complete list of main menu items for the Customize menu settings screen.
     *
     * <p>This method creates a comprehensive list of all available main menu items that users can
     * customize through the menu settings. The list includes items like "New Tab", "History",
     * "Downloads", "Brave Wallet", etc., based on feature availability and device configuration.
     *
     * <p><strong>Note:</strong> This synchronous version defaults to showing all menu items since
     * it cannot check policy asynchronously. For accurate policy-based filtering, use {@link
     * #buildMainMenuModelListAsync(Callback)} instead.
     *
     * <p><strong>Note on Icons:</strong> The returned menu items do not include drawable icons
     * because {@link android.graphics.drawable.Drawable} objects cannot be parceled across activity
     * boundaries. Instead, the settings screen uses {@link
     * CustomizeBraveMenu#getDrawableResFromMenuItemId(int)} to map menu item IDs to their
     * corresponding drawable resource IDs for display.
     *
     * @param policyStates Map of menu item IDs to their disabled-by-policy state (true = disabled,
     *     false/not present = enabled)
     * @return a ModelList containing all customizable main menu items with their IDs and titles
     */
    private MVCListAdapter.ModelList buildMainMenuModelList(Map<Integer, Boolean> policyStates) {
        MVCListAdapter.ModelList modelList = new MVCListAdapter.ModelList();

        // New Tab
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.new_tab_menu_id, R.string.menu_new_tab, 0)));

        // New Incognito Tab
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.new_incognito_tab_menu_id,
                                R.string.menu_new_incognito_tab,
                                0)));

        // Add to Group
        if (ChromeFeatureList.sTabGroupParityBottomSheetAndroid.isEnabled()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.add_to_group_menu_id, R.string.menu_add_tab_to_group, 0)));
        }

        // New Window
        if (!DeviceInfo.isAutomotive()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.new_window_menu_id, R.string.menu_new_window, 0)));
        }

        // New Incognito Window
        if (IncognitoUtils.shouldOpenIncognitoAsWindow() && !DeviceInfo.isAutomotive()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.new_incognito_window_menu_id,
                                    R.string.menu_new_incognito_window,
                                    0)));
        }

        // Move to other window
        if (MultiWindowUtils.instanceSwitcherEnabled()
                && MultiWindowUtils.isMultiInstanceApi31Enabled()
                && !DeviceInfo.isAutomotive()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.move_to_other_window_menu_id,
                                    R.string.menu_move_to_other_window,
                                    0)));
        }

        // Manage windows

        // Remove the windows count placeholder.
        String manageWindows =
                mContext.getString(R.string.menu_manage_all_windows, 0)
                        .replaceAll("\\s*\\(\\d+\\)$", "");

        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildBaseModelForTextItem(R.id.manage_all_windows_menu_id)
                                .with(AppMenuItemProperties.TITLE, manageWindows)
                                .build()));

        // Open History
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.open_history_menu_id, R.string.menu_history, 0)));

        // Tinker Tank
        if (TinkerTankDelegate.isEnabled()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.tinker_tank_menu_id, R.string.menu_tinker_tank, 0)));
        }

        // Downloads
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.downloads_menu_id, R.string.menu_downloads, 0)));

        // Bookmarks
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.all_bookmarks_menu_id, R.string.menu_bookmarks, 0)));

        // Recent Tabs
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.recent_tabs_menu_id, R.string.menu_recent_tabs, 0)));

        // Add Brave specific items.
        if (ChromeFeatureList.isEnabled(BraveFeatureList.NATIVE_BRAVE_WALLET)) {
            modelList.add(buildBraveWalletItem());
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            modelList.add(buildBravePlaylistItem());
            modelList.add(buildBraveAddToPlaylistItem());
        }
        if (BraveLeoPrefUtils.isLeoEnabled()) {
            modelList.add(buildBraveLeoItem());
        }

        modelList.add(buildSetDefaultBrowserItem());

        if (BraveVpnUtils.isVpnFeatureSupported(mContext)) {
            modelList.add(buildBraveVpnItem());
            if (BraveVpnPrefUtils.isSubscriptionPurchase()
                    && !TextUtils.isEmpty(BraveVpnPrefUtils.getRegionIsoCode())) {
                modelList.add(buildBraveVpnLocationIconItem());
            }
        }
        // Add policy-controlled items based on policy states
        for (PolicyControlledMenuItem item : getPolicyControlledMenuItems()) {
            // Check if item is disabled by policy (default to false/not disabled if not in map)
            boolean isDisabled = policyStates.getOrDefault(item.mMenuItemId, false);
            if (!isDisabled && item.mIsSupportedChecker.get()) {
                modelList.add(item.mItemBuilder.get());
            }
        }

        modelList.add(buildBraveNewsItem());
        modelList.add(buildExitItem());
        return modelList;
    }

    /**
     * Builds the complete list of page action items for the Customize menu settings screen.
     *
     * <p>This method creates a comprehensive list of all available page action menu items that
     * users can customize through the menu settings. Page actions are context-sensitive menu items
     * that operate on the current page, such as "Share", "Download Page", "Print", "Find in Page",
     * "Translate", etc., based on feature availability and current page context.
     *
     * <p><strong>Note on Icons:</strong> The returned menu items do not include drawable icons
     * because {@link android.graphics.drawable.Drawable} objects cannot be parceled across activity
     * boundaries. Instead, the settings screen uses {@link
     * CustomizeBraveMenu#getDrawableResFromMenuItemId(int)} to map menu item IDs to their
     * corresponding drawable resource IDs for display.
     *
     * @return a ModelList containing all customizable page action menu items with their IDs and
     *     titles
     */
    public MVCListAdapter.ModelList buildPageActionsModelList() {
        // Page Zoom
        final MVCListAdapter.ModelList modelList = new MVCListAdapter.ModelList();
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.page_zoom_id, R.string.page_zoom_menu_title, 0)));
        // Share
        if (mIsTablet) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.share_menu_id, R.string.menu_share_page, 0)));
        }

        // Download Page
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.download_page_id, R.string.menu_download_page, 0)));

        // Print
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(R.id.print_id, R.string.menu_print, 0)));

        // Price Tracking enable
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.enable_price_tracking_menu_id,
                                R.string.enable_price_tracking_menu_item,
                                0)));

        // Price Tracking disable
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.disable_price_tracking_menu_id,
                                R.string.disable_price_tracking_menu_item,
                                0)));

        // AI / AI PDF
        if (ChromeFeatureList.isEnabled(
                ChromeFeatureList.ADAPTIVE_BUTTON_IN_TOP_TOOLBAR_PAGE_SUMMARY)) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.ai_web_menu_id, R.string.menu_summarize_with_ai, 0)));
        }

        // Find in page
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.find_in_page_id, R.string.menu_find_in_page, 0)));

        // Translate
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.translate_id, R.string.menu_translate, 0)));

        // Shred
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SHRED)) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.brave_shred_id, R.string.brave_menu_shred_text, 0)));
        }

        // Read aloud
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.readaloud_menu_id, R.string.menu_listen_to_this_page, 0)));

        // Reader mode
        if (DomDistillerFeatures.showAlwaysOnEntryPoint()
                || DomDistillerFeatures.sReaderModeDistillInApp.isEnabled()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.reader_mode_menu_id, R.string.show_reading_mode_text, 0)));
        }

        // Open with…
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.open_with_id, R.string.menu_open_with, 0)));

        // Universal Install / Open Web APK
        if (WebappsUtils.isAddToHomeIntentSupported()) {
            // This is the 'webapp is already installed' case, so we offer to open the webapp.
            String appName = mContext.getString(R.string.webapp);
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuItemType.STANDARD,
                            buildBaseModelForTextItem(R.id.open_webapk_id)
                                    .with(
                                            AppMenuItemProperties.TITLE,
                                            mContext.getString(R.string.menu_open_webapk, appName))
                                    .build()));

            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.universal_install, R.string.menu_add_to_homescreen, 0)));
        }

        // RDS
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.reader_mode_prefs_id, R.string.menu_reader_mode_prefs, 0)));

        // Auto Dark
        if (ChromeFeatureList.isEnabled(
                ChromeFeatureList.DARKEN_WEBSITES_CHECKBOX_IN_THEMES_SETTING)) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.auto_dark_web_contents_id,
                                    R.string.menu_auto_dark_web_contents,
                                    0)));
        }

        // Paint Preview
        if (ChromeFeatureList.sPaintPreviewDemo.isEnabled()) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.paint_preview_show_id,
                                    R.string.menu_paint_preview_show,
                                    0)));
        }

        // Get Image Descriptions
        modelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD,
                        buildModelForStandardMenuItem(
                                R.id.get_image_descriptions_id,
                                R.string.menu_get_image_descriptions,
                                0)));

        // Listen to the Feed
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.FEED_AUDIO_OVERVIEWS)) {
            modelList.add(
                    new MVCListAdapter.ListItem(
                            AppMenuHandler.AppMenuItemType.STANDARD,
                            buildModelForStandardMenuItem(
                                    R.id.listen_to_feed_id, R.string.menu_listen_to_feed, 0)));
        }

        return modelList;
    }

    @Override
    public MVCListAdapter.ModelList buildMenuModelList() {
        MVCListAdapter.ModelList modelList = super.buildMenuModelList();

        int menuGroup = getMenuGroup();
        if (menuGroup == MenuGroup.PAGE_MENU) {
            // Build menu without policy-controlled items initially (safer default)
            // Policy-controlled items will be added/removed asynchronously based on policy checks
            populateBravePageModeMenu(modelList);
            // Check policies and update menu dynamically for all configured policy-controlled items
            updateMenuItemsBasedOnPolicy(modelList);
        }
        // Apply Brave icons.
        maybeReplaceIcons(modelList);

        // Customize menu item visibility.
        CustomizeBraveMenu.applyCustomization(mContext.getResources(), modelList);

        return modelList;
    }

    private void populateBravePageModeMenu(MVCListAdapter.ModelList modelList) {
        // Remove items that we don't use in Brave.
        maybeRemoveMenuItems(
                modelList,
                R.id.quick_delete_menu_id,
                R.id.quick_delete_divider_line_id,
                R.id.ntp_customization_id,
                R.id.help_id);
        if (!mIsTablet) {
            maybeRemoveMenuItems(modelList, R.id.share_menu_id);
        }

        // Shred
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SHRED)) {
            Tab tab = mActivityTabProvider.get();
            if (tab != null && !tab.isIncognito() && tab.getWebContents() != null) {
                GURL lastCommittedUrl = tab.getWebContents().getLastCommittedUrl();
                if (!lastCommittedUrl.getScheme().equals(BraveUrlConstants.BRAVE_SCHEME)
                        && !lastCommittedUrl.getScheme().equals(UrlConstants.CHROME_SCHEME)
                        && !lastCommittedUrl
                                .getScheme()
                                .equals(UrlConstants.CHROME_NATIVE_SCHEME)) {
                    addMenuItemAfter(
                            modelList,
                            buildBraveShredItem(),
                            Arrays.asList(R.id.request_desktop_site_id));
                }
            }
        }

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
        if (!BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(mBraveContext)) {
            modelList.add(buildSetDefaultBrowserItem());
        }
        if (!mJunitIsTesting) {
            if (BraveVpnUtils.isVpnFeatureSupported(mBraveContext)) {
                modelList.add(buildBraveVpnItem());
                if (BraveVpnPrefUtils.isSubscriptionPurchase()
                        && !TextUtils.isEmpty(BraveVpnPrefUtils.getRegionIsoCode())) {
                    modelList.add(buildBraveVpnLocationIconItem());
                }
            }
        }
        // Policy-controlled items (like Rewards) are handled by updateMenuItemsBasedOnPolicy()
        // They are not added here to avoid showing them if policy disables them
        // The async policy check will add them if policy allows
        modelList.add(buildBraveNewsItem());
        modelList.add(buildCustomMenuItem());
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

    @Override
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

    private MVCListAdapter.ListItem buildCustomMenuItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        CustomizeBraveMenu.BRAVE_CUSTOMIZE_ITEM_ID,
                        R.string.customize_menu_title,
                        shouldShowIconBeforeItem()
                                ? org.chromium.brave.browser.customize_menu.R.drawable
                                        .ic_window_screwdriver
                                : 0));
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

    private MVCListAdapter.ListItem buildBraveLeoItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_leo_id,
                        R.string.menu_brave_leo,
                        shouldShowIconBeforeItem() ? R.drawable.ic_brave_ai : 0));
    }

    private MVCListAdapter.ListItem buildBraveShredItem() {
        return new MVCListAdapter.ListItem(
                AppMenuHandler.AppMenuItemType.STANDARD,
                buildModelForStandardMenuItem(
                        R.id.brave_shred_id,
                        R.string.brave_menu_shred_text,
                        shouldShowIconBeforeItem() ? R.drawable.ic_brave_shred : 0));
    }

    private MVCListAdapter.ListItem buildBraveVpnItem() {
        return new MVCListAdapter.ListItem(
                AppMenuItemType.TITLE_BUTTON,
                buildModelForMenuItemWithCheckbox(
                        R.id.request_brave_vpn_id,
                        R.string.brave_vpn,
                        shouldShowIconBeforeItem() ? R.drawable.ic_vpn : 0,
                        R.id.request_brave_vpn_check_id,
                        BraveVpnProfileUtils.getInstance().isBraveVPNConnected(mBraveContext)));
    }

    private MVCListAdapter.ListItem buildBraveVpnLocationIconItem() {
        String serverLocation = " %s  %s - %s";
        String regionName =
                BraveVpnPrefUtils.getRegionPrecision()
                                .equals(BraveVpnConstants.REGION_PRECISION_COUNTRY)
                        ? mBraveContext.getString(R.string.optimal_text)
                        : BraveVpnPrefUtils.getRegionNamePretty();
        Drawable secondaryActionIcon =
                AppCompatResources.getDrawable(mBraveContext, R.drawable.ic_chevron_right);
        secondaryActionIcon = DrawableCompat.wrap(secondaryActionIcon);
        DrawableCompat.setTint(
                secondaryActionIcon,
                ContextCompat.getColor(mBraveContext, R.color.vpn_timer_icon_color));
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

    /**
     * Method to ensure that the object is created for junit tests to avoid calling the native
     * portion of code.
     *
     * @param isJunitTesting flag indicating whether the native code should be avoided.
     */
    @VisibleForTesting
    public void setIsJunitTesting(boolean isJunitTesting) {
        mJunitIsTesting = isJunitTesting;
    }
}

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Context;
import android.support.test.filters.SmallTest;
import android.view.View;
import android.view.ViewGroup;

import androidx.appcompat.app.AppCompatActivity;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.feed.webfeed.WebFeedBridge;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.findinpage.FindToolbarManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.init.StartupTabPreloader;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.AsyncTabParamsManager;
import org.chromium.chrome.browser.tabmodel.ChromeTabCreator;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.top.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.top.ToolbarControlContainer;
import org.chromium.chrome.browser.ui.TabObscuringHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.components.embedder_support.browser_context.BrowserContextHandle;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.PropertyModel;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;

/**
 * Tests to check whether classes, methods and fields exist for bytecode manipulation.
 * See classes from 'brave/build/android/bytecode/java/org/brave/bytecode' folder.
 * Classes, methods and fields should be whitelisted in 'brave/android/java/apk_for_test.flags'.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class BytecodeTest {
    @Test
    @SmallTest
    public void testClassesExist() throws Exception {
        Assert.assertTrue(classExists("org/chromium/chrome/browser/settings/MainSettings"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/sync/AndroidSyncSettings"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/LaunchIntentDispatcher"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/NewTabPageLayout"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/NewTabPage"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/sync/settings/ManageSyncSettings"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings"));
        Assert.assertTrue(classExists("org/chromium/base/CommandLineInitUtil"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ui/appmenu/AppMenu"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/bottom/BottomControlsCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/toolbar/ToolbarManager"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/toolbar/BraveToolbarManager"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTCoordinatorPhone"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/customtabs/features/toolbar/CustomTabToolbar"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/suggestions/tile/SuggestionsTileView"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/download/MimeUtils"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/app/ChromeActivity"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ChromeTabbedActivity"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/appmenu/BraveTabbedAppMenuPropertiesDelegate"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tabmodel/ChromeTabCreator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tabmodel/BraveTabCreator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BraveBookmarkUtils"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/safe_browsing/settings/BraveStandardProtectionSettingsFragment"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/safe_browsing/settings/StandardProtectionSettingsFragment"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/bottom/BraveBottomControlsMediator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/toolbar/top/ToolbarPhone"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/password_manager/settings/PasswordSettings"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/password_manager/settings/BravePasswordSettingsBase"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/app/appmenu/AppMenuPropertiesDelegateImpl"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/customtabs/CustomTabAppMenuPropertiesDelegate"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/IncognitoToggleTabLayout"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/BraveIncognitoToggleTabLayout"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabGroupUiCoordinator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/site_settings/BraveSiteSettingsDelegate"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/site_settings/ChromeSiteSettingsDelegate"));
        Assert.assertTrue(classExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings"));
        Assert.assertTrue(
                classExists("org/chromium/components/permissions/BravePermissionDialogDelegate"));
        Assert.assertTrue(
                classExists("org/chromium/components/permissions/PermissionDialogDelegate"));
        Assert.assertTrue(
                classExists("org/chromium/components/permissions/BravePermissionDialogModel"));
        Assert.assertTrue(classExists("org/chromium/components/permissions/PermissionDialogModel"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/compositor/layouts/LayoutManagerChromePhone"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/compositor/layouts/BraveLayoutManagerChrome"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tasks/tab_management/TabUiFeatureUtilities"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiFeatureUtilities"));
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/sync/AndroidSyncSettings",
                "updateCachedSettings", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/sync/AndroidSyncSettings",
                "setChromeSyncEnabled", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "extensiveBookmarkChangesBeginning", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "extensiveBookmarkChangesEnded", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "createBookmarkItem", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/LaunchIntentDispatcher",
                "isCustomTabIntent", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/homepage/HomepageManager",
                "shouldCloseAppWithZeroTabs", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ntp/NewTabPageLayout",
                "insertSiteSectionView", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ntp/NewTabPageLayout",
                "updateTileGridPlaceholderVisibility", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/query_tiles/QueryTileSection",
                "getMaxRowsForMostVisitedTiles", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/query_tiles/BraveQueryTileSection",
                        "getMaxRowsForMostVisitedTiles", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "getPermissionsLinkMessage", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "getSearchEngineSourceType", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "sortAndFilterUnnecessaryTemplateUrl", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/base/CommandLineInitUtil", "initCommandLine", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/ui/appmenu/AppMenu", "getPopupPosition", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "onOrientationChange", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "updateButtonStatus", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "updateBookmarkButtonStatus", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "updateReloadState", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "updateNewTabButtonVisibility", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "shouldShowIncognitoToggle", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/download/MimeUtils",
                "canAutoOpenMimeType", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BraveBookmarkUtils",
                "addOrEditBookmark", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkUtils",
                "showBookmarkBottomSheet", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkUtils",
                "addBookmarkAndShowSnackbar", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/components/permissions/BravePermissionDialogModel",
                        "getModel", false, null));
        Assert.assertTrue(methodExists("org/chromium/components/permissions/PermissionDialogModel",
                "getModel", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings",
                "createAdapterIfNecessary", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/TabUiFeatureUtilities",
                "isGridTabSwitcherEnabled", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/TabUiFeatureUtilities",
                "isTabGroupsAndroidEnabled", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiFeatureUtilities",
                "isGridTabSwitcherEnabled", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiFeatureUtilities",
                "isTabGroupsAndroidEnabled", false, null));
    }

    @Test
    @SmallTest
    public void testMethodsForInvocationExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ChromeTabbedActivity",
                "hideOverview", true, void.class));

        // NOTE: Add new checks above. For each new check in this method add proguard exception in
        // `brave/android/java/proguard.flags` file under `Add methods for invocation below`
        // section. Both test and regular apks should have the same exceptions.
    }

    @Test
    @SmallTest
    public void testConstructorsExistAndMatch() throws Exception {
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate",
                "org/chromium/chrome/browser/appmenu/BraveTabbedAppMenuPropertiesDelegate",
                Context.class, ActivityTabProvider.class, MultiWindowModeStateDispatcher.class,
                TabModelSelector.class, ToolbarManager.class, View.class, AppMenuDelegate.class,
                OneshotSupplier.class, ObservableSupplier.class,
                WebFeedSnackbarController.FeedLauncher.class, ModalDialogManager.class,
                SnackbarManager.class, WebFeedBridge.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/tabmodel/ChromeTabCreator",
                "org/chromium/chrome/browser/tabmodel/BraveTabCreator", Activity.class,
                WindowAndroid.class, StartupTabPreloader.class, Supplier.class, boolean.class,
                ChromeTabCreator.OverviewNTPCreator.class, AsyncTabParamsManager.class,
                ObservableSupplier.class, ObservableSupplier.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "org/chromium/chrome/browser/toolbar/BraveToolbarManager", AppCompatActivity.class,
                BrowserControlsSizer.class, FullscreenManager.class, ToolbarControlContainer.class,
                CompositorViewHolder.class, Callback.class, TopUiThemeColorProvider.class,
                TabObscuringHandler.class, ObservableSupplier.class, IdentityDiscController.class,
                List.class, ActivityTabProvider.class, ScrimCoordinator.class,
                ToolbarActionModeCallback.class, FindToolbarManager.class, ObservableSupplier.class,
                ObservableSupplier.class, Supplier.class, OneshotSupplier.class,
                OneshotSupplier.class, boolean.class, ObservableSupplier.class,
                OneshotSupplier.class, ObservableSupplier.class, OneshotSupplier.class,
                OneshotSupplier.class, WindowAndroid.class, Supplier.class, Supplier.class,
                StatusBarColorController.class, AppMenuDelegate.class,
                ActivityLifecycleDispatcher.class, Supplier.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                "org/chromium/chrome/browser/toolbar/bottom/BraveBottomControlsMediator",
                WindowAndroid.class, PropertyModel.class, BrowserControlsSizer.class,
                FullscreenManager.class, int.class, ObservableSupplier.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/app/appmenu/AppMenuPropertiesDelegateImpl",
                "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl",
                Context.class, ActivityTabProvider.class, MultiWindowModeStateDispatcher.class,
                TabModelSelector.class, ToolbarManager.class, View.class, OneshotSupplier.class,
                ObservableSupplier.class));
        Assert.assertTrue(
                constructorsMatch("org/chromium/chrome/browser/settings/SettingsLauncherImpl",
                        "org/chromium/chrome/browser/settings/BraveSettingsLauncherImpl"));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator",
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabGroupUiCoordinator",
                ViewGroup.class, ThemeColorProvider.class, ScrimCoordinator.class,
                ObservableSupplier.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/site_settings/ChromeSiteSettingsDelegate",
                "org/chromium/chrome/browser/site_settings/BraveSiteSettingsDelegate",
                Context.class, BrowserContextHandle.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/components/browser_ui/notifications/NotificationManagerProxyImpl",
                "org/chromium/chrome/browser/notifications/BraveNotificationManagerProxyImpl",
                Context.class));
    }

    @Test
    @SmallTest
    public void testFieldsExist() throws Exception {
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "mIsSyncable"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/AndroidSyncSettings", "mChromeSyncEnabled"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/AndroidSyncSettings", "mMasterSyncEnabled"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/ntp/NewTabPageLayout", "mSiteSectionView"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPageLayout", "mTileGroup"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mActivity"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mScrollViewForPolicy"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mNtpHeader"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mRootView"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mNewTabPageLayout"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mFeedSurfaceProvider"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor",
                "mHasClearedOmniboxForFocus"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/sync/settings/ManageSyncSettings",
                        "mGoogleActivityControls"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/settings/ManageSyncSettings", "mSyncEncryption"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/settings/ManageSyncSettings", "mManageSyncData"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/sync/settings/ManageSyncSettings",
                        "mSyncPaymentsIntegration"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/bottom/BottomControlsCoordinator",
                        "mMediator"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "mBottomControlsCoordinatorSupplier"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mCallbackController"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mBrowserControlsSizer"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mFullscreenManager"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mActivityTabProvider"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mAppThemeColorProvider"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mScrimCoordinator"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mShowStartSurfaceSupplier"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mMenuButtonCoordinator"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mToolbarTabController"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager", "mLocationBar"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mActionModeController"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mLocationBarModel"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager", "mToolbar"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mBookmarkBridgeSupplier"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mLayoutManager"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "mOverlayPanelVisibilitySupplier"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mTabModelSelector"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mIncognitoStateProvider"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mTabCountProvider"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager", "mTabGroupUi"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "mTabSwitcherModeCoordinatorPhone"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "mOptionalButtonController"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTCoordinatorPhone",
                "mTabSwitcherModeToolbar"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "mNewTabViewButton"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "mNewTabImageButton"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "mToggleTabStackButton"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "mShouldShowNewTabVariation"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/app/ChromeActivity", "mTabModelProfileSupplier"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                        "mBottomControlsHeight"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator", "mModel"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                        "mBrowserControlsSizer"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/IncognitoToggleTabLayout",
                        "mIncognitoButtonIcon"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator",
                "mToolbarView"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings",
                "mSearchEngineAdapter"));
    }

    @Test
    @SmallTest
    public void testSuperNames() throws Exception {
        Assert.assertTrue(checkSuperName("org/chromium/chrome/browser/settings/MainSettings",
                "org/chromium/chrome/browser/settings/BraveMainPreferencesBase"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/ntp/NewTabPageLayout", "android/widget/FrameLayout"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/password_manager/settings/PasswordSettings",
                "org/chromium/chrome/browser/password_manager/settings/BravePasswordSettingsBase"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "org/chromium/chrome/browser/search_engines/settings/BraveBaseSearchEngineAdapter"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "org/chromium/components/browser_ui/site_settings/BraveSingleCategorySettings"));
        Assert.assertTrue(checkSuperName("org/chromium/chrome/browser/ChromeTabbedActivity",
                "org/chromium/chrome/browser/app/BraveActivity"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate",
                "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/customtabs/CustomTabAppMenuPropertiesDelegate",
                "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl"));
        Assert.assertTrue(
                checkSuperName("org/chromium/chrome/browser/suggestions/tile/SuggestionsTileView",
                        "org/chromium/chrome/browser/suggestions/tile/BraveTileView"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/customtabs/features/toolbar/CustomTabToolbar",
                "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayout"));
        Assert.assertTrue(checkSuperName("org/chromium/chrome/browser/toolbar/top/ToolbarPhone",
                "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayout"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/chrome/browser/compositor/layouts/LayoutManagerChromePhone",
                "org/chromium/chrome/browser/compositor/layouts/BraveLayoutManagerChrome"));
    }

    private boolean classExists(String className) {
        return getClassForPath(className) != null;
    }

    private boolean methodExists(String className, String methodName, Boolean checkTypes,
            Class<?> returnType, Class<?>... parameterTypes) {
        Class c = getClassForPath(className);
        if (c == null) {
            return false;
        }
        for (Method m : c.getDeclaredMethods()) {
            if (m.getName().equals(methodName)) {
                if (checkTypes) {
                    Class<?> type = m.getReturnType();
                    if (type == null && returnType != null || type != null && returnType == null
                            || type != null && returnType != null && !type.equals(returnType)) {
                        return false;
                    }
                    Class<?>[] types = m.getParameterTypes();
                    if (types == null && parameterTypes != null
                            || types != null && parameterTypes == null
                            || types != null && parameterTypes != null
                                    && types.length != parameterTypes.length) {
                        return false;
                    }
                    for (int i = 0; i < (types == null ? 0 : types.length); i++) {
                        if (!types[i].equals(parameterTypes[i])) {
                            return false;
                        }
                    }
                }
                return true;
            }
        }
        return false;
    }

    private boolean fieldExists(String className, String fieldName) {
        Class c = getClassForPath(className);
        if (c == null) {
            return false;
        }
        for (Field f : c.getDeclaredFields()) {
            if (f.getName().equals(fieldName)) {
                return true;
            }
        }
        return false;
    }

    private Class getClassForPath(String path) {
        try {
            Class c = Class.forName(path.replace("/", "."));
            return c;
        } catch (ClassNotFoundException e) {
            return null;
        }
    }

    private boolean constructorsMatch(
            String class1Name, String class2Name, Class<?>... parameterTypes) {
        Class c1 = getClassForPath(class1Name);
        Class c2 = getClassForPath(class2Name);
        if (c1 == null || c2 == null) {
            return false;
        }
        try {
            Constructor ctor1 = c1.getDeclaredConstructor(parameterTypes);
            Constructor ctor2 = c2.getDeclaredConstructor(parameterTypes);
            if (ctor1 != null && ctor2 != null) {
                return true;
            }
        } catch (NoSuchMethodException e) {
            return false;
        }
        return false;
    }

    private boolean checkSuperName(String className, String superName) {
        Class c = getClassForPath(className);
        Class s = getClassForPath(superName);
        if (c == null || s == null) {
            return false;
        }
        return c.getSuperclass().equals(s);
    }
}

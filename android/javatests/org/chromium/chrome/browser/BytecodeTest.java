/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.os.Handler;
import android.view.View;
import android.view.ViewGroup;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.supplier.BooleanSupplier;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.feed.FeedActionDelegate;
import org.chromium.chrome.browser.feed.FeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.SnapScrollHelper;
import org.chromium.chrome.browser.feed.sort_ui.FeedOptionsCoordinator;
import org.chromium.chrome.browser.feed.webfeed.WebFeedBridge;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.findinpage.FindToolbarManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.init.StartupTabPreloader;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.SearchEngineLogoUtils;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.status.PageInfoIPHController;
import org.chromium.chrome.browser.omnibox.suggestions.AutocompleteDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxPedalDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.UrlBarDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.suggestions.mostvisited.ExploreIconProvider;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegateImpl;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.AsyncTabParamsManager;
import org.chromium.chrome.browser.tabmodel.ChromeTabCreator;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.toolbar.top.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.top.ToolbarControlContainer;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.toolbar.top.ToolbarTablet.OfflineDownloader;
import org.chromium.chrome.browser.ui.TabObscuringHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.site_settings.ContentSettingException;
import org.chromium.components.browser_ui.site_settings.PermissionInfo;
import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory;
import org.chromium.components.browser_ui.site_settings.Website;
import org.chromium.components.browser_ui.site_settings.WebsiteAddress;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.components.permissions.PermissionDialogController;
import org.chromium.content_public.browser.BrowserContextHandle;
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
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/LaunchIntentDispatcher"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/NewTabPageLayout"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/feed/BraveFeedSurfaceCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/feed/FeedSurfaceMediator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/feed/BraveFeedSurfaceMediator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/NewTabPage"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/BraveNewTabPage"));
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
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/top/BraveTopToolbarCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTCoordinator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar"));
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
        Assert.assertTrue(classExists("org/chromium/chrome/browser/omnibox/status/StatusMediator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/omnibox/status/BraveStatusMediator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/menu_button/MenuButtonCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/menu_button/BraveMenuButtonCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/theme/ThemeUtils"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/share/ShareDelegateImpl"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/share/BraveShareDelegateImpl"));
        Assert.assertTrue(classExists(
                "org/chromium/components/browser_ui/site_settings/ContentSettingsResources$ResourceItem"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiThemeProvider"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/autofill/AutofillPopupBridge"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/autofill/BraveAutofillPopupBridge"));
        Assert.assertTrue(
                classExists("org/chromium/components/variations/firstrun/VariationsSeedFetcher"));
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
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
                "getSearchEngineSourceType", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "sortAndFilterUnnecessaryTemplateUrl", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/base/CommandLineInitUtil", "initCommandLine", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/ui/appmenu/AppMenu", "getPopupPosition", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ui/appmenu/AppMenu",
                "runMenuItemEnterAnimations", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/ui/default_browser_promo/DefaultBrowserPromoUtils",
                "prepareLaunchPromoIfNeeded", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/ui/default_browser_promo/BraveDefaultBrowserPromoUtils",
                "prepareLaunchPromoIfNeeded", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "onOrientationChange", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "updateBookmarkButtonStatus", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "updateReloadState", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "updateNewTabButtonVisibility", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "getToolbarColorForCurrentState", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
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
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator",
                "isReliabilityLoggingEnabled", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/feed/BraveFeedSurfaceCoordinator",
                        "isReliabilityLoggingEnabled", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/feed/FeedSurfaceMediator",
                "destroyPropertiesForStream", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/theme/ThemeUtils",
                "getTextBoxColorForToolbarBackgroundInNonNativePage", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/WebsitePermissionsFetcher",
                "getPermissionsType", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "onOptionsItemSelected", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "getAddExceptionDialogMessage", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "resetList", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/ContentSettingsResources",
                "getResourceItem", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "getPreferenceKey", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "setupContentSettingsPreferences", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "setupContentSettingsPreference", false, null));
        Assert.assertTrue(methodExists("org/chromium/components/browser_ui/site_settings/Website",
                "setContentSetting", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider",
                        "getTitleTextColor", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider",
                        "getActionButtonTintList", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider",
                        "getCardViewBackgroundColor", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiThemeProvider",
                "getTitleTextColor", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiThemeProvider",
                "getActionButtonTintList", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ntp/NewTabPage",
                "updateSearchProviderHasLogo", false, null));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiThemeProvider",
                "getCardViewBackgroundColor", false, null));
        Assert.assertTrue(
                methodExists("org/chromium/components/variations/firstrun/VariationsSeedFetcher",
                        "get", false, null));
    }

    @Test
    @SmallTest
    public void testMethodsForInvocationExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ChromeTabbedActivity",
                "hideOverview", true, void.class));

        // Check for method type declaration changes here
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/BraveContentSettingsResources",
                "getResourceItem", true,
                getClassForPath(
                        "org/chromium/components/browser_ui/site_settings/ContentSettingsResources$ResourceItem"),
                int.class));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/ContentSettingsResources",
                "getResourceItem", true,
                getClassForPath(
                        "org/chromium/components/browser_ui/site_settings/ContentSettingsResources$ResourceItem"),
                int.class));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "getAddExceptionDialogMessage", true, String.class));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "resetList", true, void.class));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "getPreferenceKey", true, String.class, int.class));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "setupContentSettingsPreferences", true, void.class));
        Assert.assertTrue(methodExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "setupContentSettingsPreference", true, void.class, Preference.class, Integer.class,
                boolean.class));
        Assert.assertTrue(methodExists("org/chromium/components/browser_ui/site_settings/Website",
                "getPermissionInfo", true, PermissionInfo.class, int.class));
        Assert.assertTrue(methodExists("org/chromium/components/browser_ui/site_settings/Website",
                "getContentSettingException", true, ContentSettingException.class, int.class));
        Assert.assertTrue(methodExists("org/chromium/components/browser_ui/site_settings/Website",
                "getAddress", true, WebsiteAddress.class));
        Assert.assertTrue(methodExists("org/chromium/components/browser_ui/site_settings/Website",
                "setContentSettingException", true, void.class, int.class,
                ContentSettingException.class));
        Assert.assertTrue(methodExists("org/chromium/components/browser_ui/site_settings/Website",
                "setContentSetting", true, void.class, BrowserContextHandle.class, int.class,
                int.class));
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
                OneshotSupplier.class, OneshotSupplier.class, ObservableSupplier.class,
                WebFeedSnackbarController.FeedLauncher.class, ModalDialogManager.class,
                SnackbarManager.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/tabmodel/ChromeTabCreator",
                "org/chromium/chrome/browser/tabmodel/BraveTabCreator", Activity.class,
                WindowAndroid.class, StartupTabPreloader.class, Supplier.class, boolean.class,
                ChromeTabCreator.OverviewNTPCreator.class, AsyncTabParamsManager.class,
                Supplier.class, Supplier.class));
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
                ActivityLifecycleDispatcher.class, Supplier.class, BottomSheetController.class,
                Supplier.class, TabContentManager.class, TabCreatorManager.class,
                OneshotSupplier.class, SnackbarManager.class, JankTracker.class, Supplier.class,
                OneshotSupplier.class, OmniboxPedalDelegate.class, boolean.class));
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
                OneshotSupplier.class, ObservableSupplier.class));
        Assert.assertTrue(
                constructorsMatch("org/chromium/chrome/browser/settings/SettingsLauncherImpl",
                        "org/chromium/chrome/browser/settings/BraveSettingsLauncherImpl"));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator",
                "org/chromium/chrome/browser/tasks/tab_management/BraveTabGroupUiCoordinator",
                Activity.class, ViewGroup.class, IncognitoStateProvider.class,
                ScrimCoordinator.class, ObservableSupplier.class, BottomSheetController.class,
                ActivityLifecycleDispatcher.class, Supplier.class, TabModelSelector.class,
                TabContentManager.class, ViewGroup.class, Supplier.class, TabCreatorManager.class,
                Supplier.class, OneshotSupplier.class, SnackbarManager.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/site_settings/ChromeSiteSettingsDelegate",
                "org/chromium/chrome/browser/site_settings/BraveSiteSettingsDelegate",
                Context.class, Profile.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/components/browser_ui/notifications/NotificationManagerProxyImpl",
                "org/chromium/chrome/browser/notifications/BraveNotificationManagerProxyImpl",
                Context.class));
        Assert.assertTrue(
                constructorsMatch("org/chromium/chrome/browser/omnibox/status/StatusMediator",
                        "org/chromium/chrome/browser/omnibox/status/BraveStatusMediator",
                        PropertyModel.class, Resources.class, Context.class,
                        UrlBarEditingTextStateProvider.class, boolean.class,
                        LocationBarDataProvider.class, PermissionDialogController.class,
                        SearchEngineLogoUtils.class, OneshotSupplier.class, Supplier.class,
                        PageInfoIPHController.class, WindowAndroid.class, Supplier.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/ntp/NewTabPage",
                "org/chromium/chrome/browser/ntp/BraveNewTabPage", Activity.class,
                BrowserControlsStateProvider.class, Supplier.class, SnackbarManager.class,
                ActivityLifecycleDispatcher.class, TabModelSelector.class, boolean.class,
                NewTabPageUma.class, boolean.class, NativePageHost.class, Tab.class, String.class,
                BottomSheetController.class, Supplier.class, WindowAndroid.class, JankTracker.class,
                Supplier.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor",
                "org/chromium/chrome/browser/omnibox/suggestions/editurl/BraveEditUrlSuggestionProcessor",
                Context.class, SuggestionHost.class, UrlBarDelegate.class, Supplier.class,
                Supplier.class, Supplier.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                "org/chromium/chrome/browser/toolbar/top/BraveTopToolbarCoordinator",
                ToolbarControlContainer.class, ToolbarLayout.class, ToolbarDataProvider.class,
                ToolbarTabController.class, UserEducationHelper.class, List.class,
                OneshotSupplier.class, ThemeColorProvider.class, ThemeColorProvider.class,
                MenuButtonCoordinator.class, MenuButtonCoordinator.class, ObservableSupplier.class,
                ObservableSupplier.class, ObservableSupplier.class, ObservableSupplier.class,
                ObservableSupplier.class, ObservableSupplier.class, Callback.class, Supplier.class,
                Supplier.class, ObservableSupplier.class, BooleanSupplier.class, boolean.class,
                boolean.class, boolean.class, boolean.class, HistoryDelegate.class,
                BooleanSupplier.class, OfflineDownloader.class, boolean.class,
                ObservableSupplier.class, Callback.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/toolbar/menu_button/MenuButtonCoordinator",
                "org/chromium/chrome/browser/toolbar/menu_button/BraveMenuButtonCoordinator",
                OneshotSupplier.class, BrowserStateBrowserControlsVisibilityDelegate.class,
                WindowAndroid.class, MenuButtonCoordinator.SetFocusFunction.class, Runnable.class,
                boolean.class, Supplier.class, ThemeColorProvider.class, Supplier.class,
                Runnable.class, int.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/share/ShareDelegateImpl",
                "org/chromium/chrome/browser/share/BraveShareDelegateImpl",
                BottomSheetController.class, ActivityLifecycleDispatcher.class, Supplier.class,
                Supplier.class, ShareDelegateImpl.ShareSheetDelegate.class, boolean.class));
        Assert.assertTrue(
                constructorsMatch("org/chromium/chrome/browser/autofill/AutofillPopupBridge",
                        "org/chromium/chrome/browser/autofill/BraveAutofillPopupBridge", View.class,
                        long.class, WindowAndroid.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediator",
                Context.class, AutocompleteDelegate.class, UrlBarEditingTextStateProvider.class,
                PropertyModel.class, Handler.class, Supplier.class, Supplier.class, Supplier.class,
                LocationBarDataProvider.class, Callback.class, Supplier.class, BookmarkState.class,
                JankTracker.class, ExploreIconProvider.class, OmniboxPedalDelegate.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/feed/FeedSurfaceMediator",
                "org/chromium/chrome/browser/feed/BraveFeedSurfaceMediator",
                FeedSurfaceCoordinator.class, Context.class, SnapScrollHelper.class,
                PropertyModel.class, int.class, FeedActionDelegate.class,
                FeedOptionsCoordinator.class));
    }

    @Test
    @SmallTest
    public void testFieldsExist() throws Exception {
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/ntp/NewTabPageLayout", "mSiteSectionView"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPageLayout", "mTileGroup"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mNtpHeader"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mRootView"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceMediator", "mCoordinator"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/feed/FeedSurfaceMediator", "mSnapScrollHelper"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/ntp/NewTabPage", "mBrowserControlsStateProvider"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mNewTabPageLayout"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mFeedSurfaceProvider"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mToolbarSupplier"));
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
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/settings/ManageSyncSettings", "mTurnOffSync"));
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
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mBottomSheetController"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "mActivityLifecycleDispatcher"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mIsWarmOnResumeSupplier"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mTabContentManager"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mTabCreatorManager"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "mOverviewModeBehaviorSupplier"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mSnackbarManager"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "mTabSwitcherModeCoordinator"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "mOptionalButtonController"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTCoordinator",
                        "mTabSwitcherModeToolbar"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "mNewTabViewButton"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "mNewTabImageButton"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "mToggleTabStackButton"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "mShouldShowNewTabVariation"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar",
                        "mIsIncognito"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/app/ChromeActivity",
                "mBrowserControlsManagerSupplier"));
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
        Assert.assertTrue(fieldExists(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "mCategory", true, SiteSettingsCategory.class));
        Assert.assertTrue(fieldExists(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings", "mSite",
                true, Website.class));
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
                "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl"));
        Assert.assertTrue(checkSuperName("org/chromium/chrome/browser/toolbar/top/ToolbarPhone",
                "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl"));
        Assert.assertTrue(checkSuperName("org/chromium/chrome/browser/toolbar/top/ToolbarTablet",
                "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                "org/chromium/components/browser_ui/site_settings/BraveSingleCategorySettings"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                "org/chromium/components/browser_ui/site_settings/BraveSingleWebsiteSettings"));
        Assert.assertTrue(checkSuperName("org/chromium/components/browser_ui/site_settings/Website",
                "org/chromium/components/browser_ui/site_settings/BraveWebsite"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/components/browser_ui/site_settings/FourStateCookieSettingsPreference",
                "org/chromium/components/browser_ui/site_settings/BraveFourStateCookieSettingsPreferenceBase"));
        Assert.assertTrue(checkSuperName(
                "org/chromium/components/browser_ui/site_settings/SiteSettings",
                "org/chromium/components/browser_ui/site_settings/BraveSiteSettingsPreferencesBase"));
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
        return fieldExists(className, fieldName, false, null);
    }

    private boolean fieldExists(
            String className, String fieldName, Boolean checkTypes, Class<?> fieldType) {
        Class c = getClassForPath(className);
        if (c == null) {
            return false;
        }
        for (Field f : c.getDeclaredFields()) {
            if (f.getName().equals(fieldName)) {
                if (checkTypes) {
                    if (fieldType != null && f.getType().equals(fieldType)) return true;
                } else
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

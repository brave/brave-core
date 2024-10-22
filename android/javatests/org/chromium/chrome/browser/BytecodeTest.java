/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.ActionMode;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.Window;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.recyclerview.widget.RecyclerView;
import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.FeatureMap;
import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.shared_preferences.PreferenceKeyRegistry;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.supplier.Supplier;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.bookmarks.BookmarkImageFetcher;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.BookmarkMoveSnackbarManager;
import org.chromium.chrome.browser.bookmarks.BookmarkOpener;
import org.chromium.chrome.browser.bookmarks.BookmarkUiPrefs;
import org.chromium.chrome.browser.bookmarks.BookmarkUndoController;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.layouts.LayoutRenderHost;
import org.chromium.chrome.browser.compositor.layouts.LayoutUpdateHost;
import org.chromium.chrome.browser.customtabs.features.partialcustomtab.BravePartialCustomTabBottomSheetStrategy;
import org.chromium.chrome.browser.customtabs.features.partialcustomtab.PartialCustomTabHandleStrategyFactory;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.feed.FeedActionDelegate;
import org.chromium.chrome.browser.feed.FeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.SnapScrollHelper;
import org.chromium.chrome.browser.feed.sort_ui.FeedOptionsCoordinator;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.findinpage.FindToolbarManager;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.hub.ResourceButtonData;
import org.chromium.chrome.browser.keyboard_accessory.ManualFillingComponentSupplier;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.logo.CachedTintedBitmap;
import org.chromium.chrome.browser.logo.LogoCoordinator;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.new_tab_url.DseNewTabUrlManager;
import org.chromium.chrome.browser.notifications.BraveNotificationPlatformBridge;
import org.chromium.chrome.browser.notifications.NotificationBuilderBase;
import org.chromium.chrome.browser.notifications.NotificationPlatformBridge.NotificationIdentifyingAttributes;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.omnibox.BackKeyBehaviorDelegate;
import org.chromium.chrome.browser.omnibox.BraveLocationBarMediator;
import org.chromium.chrome.browser.omnibox.DeferredIMEWindowInsetApplicationCallback;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.LocationBarEmbedderUiOverrides;
import org.chromium.chrome.browser.omnibox.LocationBarLayout;
import org.chromium.chrome.browser.omnibox.OverrideUrlLoadingDelegate;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.status.PageInfoIPHController;
import org.chromium.chrome.browser.omnibox.status.StatusCoordinator.PageInfoAction;
import org.chromium.chrome.browser.omnibox.suggestions.AutocompleteDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdownEmbedder;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdownScrollListener;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegateImpl;
import org.chromium.chrome.browser.suggestions.tile.MostVisitedTilesLayout;
import org.chromium.chrome.browser.suggestions.tile.TileRenderer;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabContextMenuItemDelegate;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tabmodel.AsyncTabParamsManager;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.HomeSurfaceTracker;
import org.chromium.chrome.browser.tasks.tab_management.TabSwitcherPaneCoordinatorFactory;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.toolbar.top.ToggleTabStackButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.top.ToolbarControlContainer;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.toolbar.top.ToolbarTablet.OfflineDownloader;
import org.chromium.chrome.browser.ui.appmenu.AppMenuBlocker;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateProvider;
import org.chromium.components.browser_ui.edge_to_edge.EdgeToEdgeStateProvider;
import org.chromium.components.browser_ui.site_settings.ContentSettingException;
import org.chromium.components.browser_ui.site_settings.PermissionInfo;
import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory;
import org.chromium.components.browser_ui.site_settings.Website;
import org.chromium.components.browser_ui.site_settings.WebsiteAddress;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.components.browser_ui.widget.dragreorder.DragReorderableRecyclerViewAdapter;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.components.browser_ui.widget.selectable_list.SelectableListLayout;
import org.chromium.components.browser_ui.widget.selectable_list.SelectableListToolbar.SearchDelegate;
import org.chromium.components.browser_ui.widget.selectable_list.SelectionDelegate;
import org.chromium.components.commerce.core.ShoppingService;
import org.chromium.components.embedder_support.contextmenu.ContextMenuNativeDelegate;
import org.chromium.components.embedder_support.contextmenu.ContextMenuParams;
import org.chromium.components.external_intents.ExternalNavigationDelegate;
import org.chromium.components.externalauth.ExternalAuthUtils;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.components.permissions.PermissionDialogController;
import org.chromium.components.signin.base.AccountInfo;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.InsetObserver;
import org.chromium.ui.ViewProvider;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.IntentRequestTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.WindowDelegate;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.List;
import java.util.Optional;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;
import java.util.function.DoubleConsumer;
import java.util.function.Function;

/**
 * Tests to check whether classes, methods and fields exist for bytecode manipulation. See classes
 * from 'brave/build/android/bytecode/java/org/brave/bytecode' folder. Classes, methods and fields
 * should be whitelisted in 'brave/android/java/apk_for_test.flags'.
 */
@Batch(Batch.PER_CLASS)
@RunWith(BaseJUnit4ClassRunner.class)
public class BytecodeTest {
    enum MethodModifier {
        REGULAR,
        STATIC
    }

    @BeforeClass
    public static void beforeClass() {
        Looper.prepare();
    }

    @Test
    @SmallTest
    public void testClassesExist() throws Exception {
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ChromeApplicationImpl"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/settings/MainSettings"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/LaunchIntentDispatcher"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/NewTabPageLayout"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/feed/FeedSurfaceMediator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ntp/NewTabPage"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/sync/settings/ManageSyncSettings"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/password_manager/settings/PasswordAccessReauthenticationHelper")); // presubmit: ignore-long-line
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings"));
        Assert.assertTrue(classExists("org/chromium/base/CommandLineInitUtil"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ui/appmenu/AppMenu"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/toolbar/bottom/BottomControlsCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/toolbar/ToolbarManager"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/customtabs/features/toolbar/CustomTabToolbar")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/suggestions/tile/SuggestionsTileView"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/download/MimeUtils"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/app/ChromeActivity"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ChromeTabbedActivity"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tabmodel/ChromeTabCreator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/safe_browsing/settings/StandardProtectionSettingsFragment"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/toolbar/top/ToolbarPhone"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/password_manager/settings/PasswordSettings"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/app/appmenu/AppMenuPropertiesDelegateImpl"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/customtabs/CustomTabAppMenuPropertiesDelegate")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/site_settings/ChromeSiteSettingsDelegate"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings"));
        Assert.assertTrue(
                classExists("org/chromium/components/permissions/PermissionDialogDelegate"));
        Assert.assertTrue(
                classExists("org/chromium/components/permissions/PermissionDialogModelFactory"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/omnibox/status/StatusMediator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/toolbar/menu_button/MenuButtonCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/theme/ThemeUtils"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/share/ShareDelegateImpl"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/components/browser_ui/site_settings/ContentSettingsResources$ResourceItem"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tab_ui/TabUiThemeUtils"));
        Assert.assertTrue(
                classExists("org/chromium/components/variations/firstrun/VariationsSeedFetcher"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/partnercustomizations/CustomizationProviderDelegateUpstreamImpl"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/share/send_tab_to_self/ManageAccountDevicesLinkView")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/dom_distiller/ReaderModeManager"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/download/DownloadMessageUiControllerImpl"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteCoordinator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/DropdownItemViewInfoListBuilder")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/DropdownItemViewInfoListManager")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/omnibox/LocationBarCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/omnibox/LocationBarLayout"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/omnibox/LocationBarPhone"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/omnibox/LocationBarTablet"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/searchwidget/SearchActivityLocationBarLayout")); // presubmit: ignore-long-line
        Assert.assertTrue(classExists("org/chromium/chrome/browser/omnibox/LocationBarMediator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tasks/ReturnToChromeUtil"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/IntentHandler"));
        Assert.assertTrue(classExists("org/chromium/components/cached_flags/CachedFlag"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/logo/LogoMediator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tracing/settings/DeveloperSettings"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/incognito/reauth/TabSwitcherIncognitoReauthCoordinator")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/incognito/reauth/FullScreenIncognitoReauthCoordinator")); // presubmit: ignore-long-line
        Assert.assertTrue(classExists("org/chromium/chrome/browser/firstrun/FreIntentCreator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/feedback/HelpAndFeedbackLauncherImpl"));
        Assert.assertTrue(
                classExists("org/chromium/components/external_intents/ExternalNavigationHandler"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/contextmenu/ChromeContextMenuPopulator"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tabmodel/TabGroupModelFilterImpl"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/SwipeRefreshHandler"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/identity_disc/IdentityDiscController"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BookmarkUiPrefs"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/multiwindow/MultiInstanceManagerApi31"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/multiwindow/MultiWindowUtils"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/password_manager/settings/ExportFlow"));
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/browsing_data/ClearBrowsingDataFragmentAdvanced")); // presubmit: ignore-long-line
        Assert.assertTrue(
                classExists(
                        "org/chromium/chrome/browser/notifications/NotificationPlatformBridge"));

        Assert.assertTrue(classExists("org/chromium/chrome/browser/settings/SettingsIntentUtil"));
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                        "extensiveBookmarkChangesBeginning",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                        "extensiveBookmarkChangesEnded",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                        "createBookmarkItem",
                        MethodModifier.STATIC,
                        false,
                        null));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/LaunchIntentDispatcher",
                        "isCustomTabIntent",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/homepage/HomepageManager",
                        "shouldCloseAppWithZeroTabs",
                        MethodModifier.REGULAR,
                        false,
                        null));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout",
                        "insertSiteSectionView",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout",
                        "setSearchProviderTopMargin",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout",
                        "setSearchProviderBottomMargin",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout",
                        "getLogoMargin",
                        MethodModifier.REGULAR,
                        true,
                        int.class,
                        boolean.class));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                        "getSearchEngineSourceType",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                        "sortAndFilterUnnecessaryTemplateUrl",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/base/CommandLineInitUtil",
                        "initCommandLine",
                        MethodModifier.STATIC,
                        false,
                        null));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ui/appmenu/AppMenu",
                        "getPopupPosition",
                        MethodModifier.STATIC,
                        false,
                        null));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ui/appmenu/AppMenu",
                        "runMenuItemEnterAnimations",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ui/default_browser_promo/DefaultBrowserPromoUtils", // presubmit: ignore-long-line
                        "prepareLaunchPromoIfNeeded",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class,
                        Activity.class,
                        WindowAndroid.class,
                        Tracker.class,
                        boolean.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "onOrientationChange",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "updateBookmarkButtonStatus",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "updateReloadState",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/download/MimeUtils",
                        "canAutoOpenMimeType",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/bookmarks/BookmarkUtils",
                        "addOrEditBookmark",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/bookmarks/BookmarkUtils",
                        "showBookmarkManagerOnPhone",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/permissions/PermissionDialogModelFactory",
                        "getModel",
                        MethodModifier.STATIC,
                        false,
                        null));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings",
                        "createAdapterIfNecessary",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/theme/ThemeUtils",
                        "getTextBoxColorForToolbarBackgroundInNonNativePage",
                        MethodModifier.STATIC,
                        true,
                        int.class,
                        Context.class,
                        int.class,
                        boolean.class,
                        boolean.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/WebsitePermissionsFetcher", // presubmit: ignore-long-line
                        "getPermissionsType",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "onOptionsItemSelected",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "getAddExceptionDialogMessage",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "resetList",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "getPreferenceKey",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "setupContentSettingsPreferences",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "setupContentSettingsPreference",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "setContentSetting",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tab_ui/TabUiThemeUtils",
                        "getTitleTextColor",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tab_ui/TabUiThemeUtils",
                        "getCardViewBackgroundColor",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider",
                        "getActionButtonTintList",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ntp/NewTabPage",
                        "updateSearchProviderHasLogo",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/variations/firstrun/VariationsSeedFetcher",
                        "get",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/suggestions/tile/MostVisitedTilesMediator",
                        "updateTilePlaceholderVisibility",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "onPrimaryColorChanged",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "updateButtonVisibility",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "shouldShowDeleteButton",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tasks/ReturnToChromeUtil",
                        "shouldShowNtpAsHomeSurfaceAtStartup",
                        MethodModifier.STATIC,
                        true,
                        boolean.class,
                        Intent.class,
                        Bundle.class,
                        ChromeInactivityTracker.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/IntentHandler",
                        "getUrlForCustomTab",
                        MethodModifier.STATIC,
                        true,
                        String.class,
                        Intent.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/IntentHandler",
                        "getUrlForWebapp",
                        MethodModifier.STATIC,
                        true,
                        String.class,
                        Intent.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/IntentHandler",
                        "isJavascriptSchemeOrInvalidUrl",
                        MethodModifier.STATIC,
                        true,
                        boolean.class,
                        String.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/IntentHandler",
                        "extractUrlFromIntent",
                        MethodModifier.STATIC,
                        true,
                        String.class,
                        Intent.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/notifications/permissions/NotificationPermissionRationaleDialogController",
                        "wrapDialogDismissalCallback",
                        MethodModifier.REGULAR,
                        true,
                        Callback.class,
                        Callback.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/download/DownloadMessageUiControllerImpl",
                        "isVisibleToUser",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/logo/LogoMediator",
                        "updateVisibility",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/contextmenu/ChromeContextMenuPopulator",
                        "onItemSelected",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/base/shared_preferences/StrictPreferenceKeyChecker",
                        "isKeyInUse",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class,
                        String.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/identity_disc/IdentityDiscController",
                        "calculateButtonData",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/share/send_tab_to_self/ManageAccountDevicesLinkView", // presubmit: ignore-long-line
                        "getSharingAccountInfo",
                        MethodModifier.STATIC,
                        true,
                        AccountInfo.class,
                        Profile.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/multiwindow/MultiInstanceManagerApi31",
                        "handleMenuOrKeyboardAction",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/multiwindow/MultiInstanceManagerApi31",
                        "moveTabAction",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/multiwindow/MultiWindowUtils",
                        "isOpenInOtherWindowSupported",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/multiwindow/MultiWindowUtils",
                        "isMoveToOtherWindowSupported",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/multiwindow/MultiWindowUtils",
                        "canEnterMultiWindowMode",
                        MethodModifier.REGULAR,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/multiwindow/MultiWindowUtils",
                        "shouldShowManageWindowsMenu",
                        MethodModifier.STATIC,
                        false,
                        null));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SiteSettingsCategory",
                        "contentSettingsType",
                        MethodModifier.STATIC,
                        true,
                        int.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SiteSettingsCategory",
                        "preferenceKey",
                        MethodModifier.STATIC,
                        true,
                        String.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/password_manager/settings/ExportFlow",
                        "runSharePasswordsIntent",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/styles/OmniboxResourceProvider",
                        "getToolbarSidePaddingForNtp",
                        MethodModifier.STATIC,
                        true,
                        int.class,
                        Context.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/undo_tab_close_snackbar/UndoBarController",
                        "showUndoBar",
                        MethodModifier.REGULAR,
                        true,
                        void.class,
                        List.class,
                        boolean.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/notifications/NotificationPlatformBridge",
                        "dispatchNotificationEvent",
                        MethodModifier.STATIC,
                        true,
                        boolean.class,
                        Intent.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/notifications/NotificationPlatformBridge",
                        "prepareNotificationBuilder",
                        MethodModifier.STATIC,
                        true,
                        NotificationBuilderBase.class,
                        NotificationIdentifyingAttributes.class,
                        boolean.class,
                        String.class,
                        String.class,
                        Bitmap.class,
                        Bitmap.class,
                        Bitmap.class,
                        int[].class,
                        long.class,
                        boolean.class,
                        boolean.class,
                        BraveNotificationPlatformBridge.getActionInfoArrayClass()));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/settings/SettingsIntentUtil",
                        "createIntent",
                        MethodModifier.STATIC,
                        true,
                        Intent.class,
                        Context.class,
                        String.class,
                        Bundle.class));
    }

    @Test
    @SmallTest
    public void testMethodsForInvocationExist() throws Exception {
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ChromeTabbedActivity",
                        "hideOverview",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/ChromeTabbedActivity",
                        "maybeHandleUrlIntent",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class,
                        Intent.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteCoordinator",
                        "createViewProvider",
                        MethodModifier.REGULAR,
                        true,
                        ViewProvider.class,
                        boolean.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "loadUrlForOmniboxMatch",
                        MethodModifier.REGULAR,
                        true,
                        void.class,
                        int.class,
                        AutocompleteMatch.class,
                        GURL.class,
                        long.class,
                        boolean.class,
                        boolean.class));

        // Check for method type declaration changes here
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/ContentSettingsResources",
                        "getResourceItem",
                        MethodModifier.STATIC,
                        true,
                        getClassForPath(
                                "org/chromium/components/browser_ui/site_settings/ContentSettingsResources$ResourceItem"),
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "getAddExceptionDialogMessage",
                        MethodModifier.REGULAR,
                        true,
                        String.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "resetList",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "getPreferenceKey",
                        MethodModifier.STATIC,
                        true,
                        String.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "setupContentSettingsPreferences",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "setupContentSettingsPreference",
                        MethodModifier.REGULAR,
                        true,
                        void.class,
                        Preference.class,
                        Integer.class,
                        boolean.class,
                        boolean.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "getPermissionInfo",
                        MethodModifier.REGULAR,
                        true,
                        PermissionInfo.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "getContentSettingException",
                        MethodModifier.REGULAR,
                        true,
                        ContentSettingException.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "getAddress",
                        MethodModifier.REGULAR,
                        true,
                        WebsiteAddress.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "setContentSettingException",
                        MethodModifier.REGULAR,
                        true,
                        void.class,
                        int.class,
                        ContentSettingException.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "setContentSetting",
                        MethodModifier.REGULAR,
                        true,
                        void.class,
                        BrowserContextHandle.class,
                        int.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tab/TabHelpers",
                        "initTabHelpers",
                        MethodModifier.STATIC,
                        true,
                        void.class,
                        Tab.class,
                        Tab.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tabmodel/TabGroupModelFilterImpl",
                        "shouldUseParentIds",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class,
                        Tab.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tabmodel/TabGroupModelFilterImpl",
                        "isTabModelRestored",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class));

        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/SwipeRefreshHandler",
                        "start",
                        MethodModifier.REGULAR,
                        true,
                        boolean.class,
                        int.class,
                        int.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/password_manager/settings/ExportFlow",
                        "runCreateFileOnDiskIntent",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor",
                        "onCopyLink",
                        MethodModifier.REGULAR,
                        true,
                        void.class,
                        AutocompleteMatch.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/hub/HubManagerImpl",
                        "ensureHubCoordinatorIsInitialized",
                        MethodModifier.REGULAR,
                        true,
                        void.class));
        Assert.assertTrue(
                methodExists(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedNavigationBarColorController", // presubmit: ignore-long-line
                        "getNavigationBarColor",
                        MethodModifier.REGULAR,
                        true,
                        int.class,
                        boolean.class));
        // NOTE: Add new checks above. For each new check in this method add proguard exception in
        // `brave/android/java/proguard.flags` file under `Add methods for invocation below`
        // section. Both test and regular apks should have the same exceptions.
    }

    @Test
    @SmallTest
    public void testConstructorsExistAndMatch() throws Exception {
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate",
                        "org/chromium/chrome/browser/appmenu/BraveTabbedAppMenuPropertiesDelegate",
                        Context.class,
                        ActivityTabProvider.class,
                        MultiWindowModeStateDispatcher.class,
                        TabModelSelector.class,
                        ToolbarManager.class,
                        View.class,
                        AppMenuDelegate.class,
                        OneshotSupplier.class,
                        ObservableSupplier.class,
                        WebFeedSnackbarController.FeedLauncher.class,
                        ModalDialogManager.class,
                        SnackbarManager.class,
                        OneshotSupplier.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/tabmodel/ChromeTabCreator",
                        "org/chromium/chrome/browser/tabmodel/BraveTabCreator",
                        Activity.class,
                        WindowAndroid.class,
                        Supplier.class,
                        OneshotSupplier.class,
                        boolean.class,
                        AsyncTabParamsManager.class,
                        Supplier.class,
                        Supplier.class,
                        DseNewTabUrlManager.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "org/chromium/chrome/browser/toolbar/BraveToolbarManager",
                        AppCompatActivity.class,
                        BottomControlsStacker.class,
                        BrowserControlsSizer.class,
                        FullscreenManager.class,
                        ObservableSupplier.class,
                        ToolbarControlContainer.class,
                        CompositorViewHolder.class,
                        Callback.class,
                        TopUiThemeColorProvider.class,
                        TabObscuringHandler.class,
                        ObservableSupplier.class,
                        List.class,
                        ActivityTabProvider.class,
                        ScrimCoordinator.class,
                        ToolbarActionModeCallback.class,
                        FindToolbarManager.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        OneshotSupplier.class,
                        OneshotSupplier.class,
                        boolean.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        OneshotSupplier.class,
                        WindowAndroid.class,
                        Supplier.class,
                        Supplier.class,
                        StatusBarColorController.class,
                        AppMenuDelegate.class,
                        ActivityLifecycleDispatcher.class,
                        BottomSheetController.class,
                        DataSharingTabManager.class,
                        TabContentManager.class,
                        TabCreatorManager.class,
                        Supplier.class,
                        OmniboxActionDelegate.class,
                        Supplier.class,
                        boolean.class,
                        BackPressManager.class,
                        ObservableSupplier.class,
                        View.class,
                        ObservableSupplier.class,
                        DesktopWindowStateProvider.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                        "org/chromium/chrome/browser/toolbar/bottom/BraveBottomControlsMediator",
                        WindowAndroid.class,
                        PropertyModel.class,
                        BottomControlsStacker.class,
                        BrowserStateBrowserControlsVisibilityDelegate.class,
                        FullscreenManager.class,
                        TabObscuringHandler.class,
                        int.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/app/appmenu/AppMenuPropertiesDelegateImpl",
                        "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl",
                        Context.class,
                        ActivityTabProvider.class,
                        MultiWindowModeStateDispatcher.class,
                        TabModelSelector.class,
                        ToolbarManager.class,
                        View.class,
                        OneshotSupplier.class,
                        ObservableSupplier.class,
                        OneshotSupplier.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/settings/SettingsNavigationImpl",
                        "org/chromium/chrome/browser/settings/BraveSettingsLauncherImpl"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator",
                        "org/chromium/chrome/browser/tasks/tab_management/BraveTabGroupUiCoordinator",
                        Activity.class,
                        ViewGroup.class,
                        BrowserControlsStateProvider.class,
                        IncognitoStateProvider.class,
                        ScrimCoordinator.class,
                        ObservableSupplier.class,
                        BottomSheetController.class,
                        DataSharingTabManager.class,
                        TabModelSelector.class,
                        TabContentManager.class,
                        TabCreatorManager.class,
                        OneshotSupplier.class,
                        ModalDialogManager.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/site_settings/ChromeSiteSettingsDelegate",
                        "org/chromium/chrome/browser/site_settings/BraveSiteSettingsDelegate",
                        Context.class,
                        Profile.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/components/browser_ui/notifications/NotificationManagerProxyImpl",
                        "org/chromium/chrome/browser/notifications/BraveNotificationManagerProxyImpl",
                        Context.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/status/StatusMediator",
                        "org/chromium/chrome/browser/omnibox/status/BraveStatusMediator",
                        PropertyModel.class,
                        Context.class,
                        UrlBarEditingTextStateProvider.class,
                        boolean.class,
                        LocationBarDataProvider.class,
                        PermissionDialogController.class,
                        OneshotSupplier.class,
                        Supplier.class,
                        PageInfoIPHController.class,
                        WindowAndroid.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/ntp/NewTabPage",
                        "org/chromium/chrome/browser/ntp/BraveNewTabPage",
                        Activity.class,
                        BrowserControlsStateProvider.class,
                        Supplier.class,
                        ModalDialogManager.class,
                        SnackbarManager.class,
                        ActivityLifecycleDispatcher.class,
                        TabModelSelector.class,
                        boolean.class,
                        NewTabPageUma.class,
                        boolean.class,
                        NativePageHost.class,
                        Tab.class,
                        String.class,
                        BottomSheetController.class,
                        Supplier.class,
                        WindowAndroid.class,
                        JankTracker.class,
                        Supplier.class,
                        HomeSurfaceTracker.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        OneshotSupplier.class,
                        ObservableSupplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "org/chromium/chrome/browser/toolbar/top/BraveTopToolbarCoordinator",
                        ToolbarControlContainer.class,
                        ToolbarLayout.class,
                        ToolbarDataProvider.class,
                        ToolbarTabController.class,
                        UserEducationHelper.class,
                        List.class,
                        OneshotSupplier.class,
                        ThemeColorProvider.class,
                        MenuButtonCoordinator.class,
                        ObservableSupplier.class,
                        ToggleTabStackButtonCoordinator.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        Supplier.class,
                        HistoryDelegate.class,
                        BooleanSupplier.class,
                        OfflineDownloader.class,
                        boolean.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        BrowserStateBrowserControlsVisibilityDelegate.class,
                        FullscreenManager.class,
                        TabObscuringHandler.class,
                        DesktopWindowStateProvider.class,
                        OneshotSupplier.class,
                        OnLongClickListener.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/toolbar/menu_button/MenuButtonCoordinator",
                        "org/chromium/chrome/browser/toolbar/menu_button/BraveMenuButtonCoordinator", // presubmit: ignore-long-line
                        OneshotSupplier.class,
                        BrowserStateBrowserControlsVisibilityDelegate.class,
                        WindowAndroid.class,
                        MenuButtonCoordinator.SetFocusFunction.class,
                        Runnable.class,
                        boolean.class,
                        Supplier.class,
                        ThemeColorProvider.class,
                        Supplier.class,
                        Runnable.class,
                        int.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/share/ShareDelegateImpl",
                        "org/chromium/chrome/browser/share/BraveShareDelegateImpl",
                        BottomSheetController.class,
                        ActivityLifecycleDispatcher.class,
                        Supplier.class,
                        Supplier.class,
                        Supplier.class,
                        ShareDelegateImpl.ShareSheetDelegate.class,
                        boolean.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediator",
                        Context.class,
                        AutocompleteDelegate.class,
                        UrlBarEditingTextStateProvider.class,
                        PropertyModel.class,
                        Handler.class,
                        Supplier.class,
                        Supplier.class,
                        Supplier.class,
                        LocationBarDataProvider.class,
                        Callback.class,
                        Supplier.class,
                        BookmarkState.class,
                        OmniboxActionDelegate.class,
                        ActivityLifecycleDispatcher.class,
                        OmniboxSuggestionsDropdownEmbedder.class,
                        WindowAndroid.class,
                        DeferredIMEWindowInsetApplicationCallback.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/multiwindow/MultiInstanceManagerApi31",
                        "org/chromium/chrome/browser/multiwindow/BraveMultiInstanceManagerApi31",
                        Activity.class,
                        ObservableSupplier.class,
                        MultiWindowModeStateDispatcher.class,
                        ActivityLifecycleDispatcher.class,
                        ObservableSupplier.class,
                        MenuOrKeyboardActionController.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/multiwindow/MultiWindowUtils",
                        "org/chromium/chrome/browser/multiwindow/BraveMultiWindowUtils"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/feed/FeedSurfaceMediator",
                        "org/chromium/chrome/browser/feed/BraveFeedSurfaceMediator",
                        FeedSurfaceCoordinator.class,
                        Context.class,
                        SnapScrollHelper.class,
                        PropertyModel.class,
                        int.class,
                        FeedActionDelegate.class,
                        FeedOptionsCoordinator.class,
                        UiConfig.class,
                        Profile.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/partnercustomizations/CustomizationProviderDelegateUpstreamImpl",
                        "org/chromium/chrome/browser/partnercustomizations/BraveCustomizationProviderDelegateImpl"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/share/send_tab_to_self/ManageAccountDevicesLinkView",
                        "org/chromium/chrome/browser/share/send_tab_to_self/BraveManageAccountDevicesLinkView",
                        Context.class,
                        AttributeSet.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/suggestions/tile/MostVisitedTilesMediator",
                        "org/chromium/chrome/browser/suggestions/tile/BraveMostVisitedTilesMediator",
                        Resources.class,
                        UiConfig.class,
                        MostVisitedTilesLayout.class,
                        ViewStub.class,
                        TileRenderer.class,
                        PropertyModel.class,
                        boolean.class,
                        Runnable.class,
                        Runnable.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/dom_distiller/ReaderModeManager",
                        "org/chromium/chrome/browser/dom_distiller/BraveReaderModeManager",
                        Tab.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/crash/ChromePureJavaExceptionReporter",
                        "org/chromium/chrome/browser/crash/BravePureJavaExceptionReporter"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/suggestions/DropdownItemViewInfoListBuilder",
                        "org/chromium/chrome/browser/omnibox/suggestions/BraveDropdownItemViewInfoListBuilder",
                        Supplier.class,
                        BookmarkState.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/suggestions/DropdownItemViewInfoListManager",
                        "org/chromium/chrome/browser/omnibox/suggestions/BraveDropdownItemViewInfoListManager",
                        ModelList.class,
                        Context.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/LocationBarCoordinator",
                        "org/chromium/chrome/browser/omnibox/BraveLocationBarCoordinator",
                        View.class,
                        View.class,
                        ObservableSupplier.class,
                        LocationBarDataProvider.class,
                        ActionMode.Callback.class,
                        WindowDelegate.class,
                        WindowAndroid.class,
                        Supplier.class,
                        Supplier.class,
                        Supplier.class,
                        IncognitoStateProvider.class,
                        ActivityLifecycleDispatcher.class,
                        OverrideUrlLoadingDelegate.class,
                        BackKeyBehaviorDelegate.class,
                        PageInfoAction.class,
                        Callback.class,
                        BraveLocationBarMediator.getSaveOfflineButtonStateClass(),
                        BraveLocationBarMediator.getOmniboxUmaClass(),
                        Supplier.class,
                        BookmarkState.class,
                        BooleanSupplier.class,
                        Supplier.class,
                        OmniboxActionDelegate.class,
                        BrowserStateBrowserControlsVisibilityDelegate.class,
                        BackPressManager.class,
                        OmniboxSuggestionsDropdownScrollListener.class,
                        ObservableSupplier.class,
                        LocationBarEmbedderUiOverrides.class,
                        View.class,
                        Supplier.class,
                        OnLongClickListener.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "org/chromium/chrome/browser/omnibox/BraveLocationBarMediator",
                        Context.class,
                        LocationBarLayout.class,
                        LocationBarDataProvider.class,
                        LocationBarEmbedderUiOverrides.class,
                        ObservableSupplier.class,
                        OverrideUrlLoadingDelegate.class,
                        BraveLocationBarMediator.getLocaleManagerClass(),
                        OneshotSupplier.class,
                        BackKeyBehaviorDelegate.class,
                        WindowAndroid.class,
                        boolean.class,
                        BraveLocationBarMediator.getLensControllerClass(),
                        BraveLocationBarMediator.getSaveOfflineButtonStateClass(),
                        BraveLocationBarMediator.getOmniboxUmaClass(),
                        BooleanSupplier.class,
                        BraveLocationBarMediator.getOmniboxSuggestionsDropdownEmbedderImplClass(),
                        ObservableSupplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/AppHooks",
                        "org/chromium/chrome/browser/BraveAppHooks"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/components/cached_flags/CachedFlag",
                        "org/chromium/components/cached_flags/BraveCachedFlag",
                        FeatureMap.class,
                        String.class,
                        boolean.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/components/cached_flags/CachedFlag",
                        "org/chromium/components/cached_flags/BraveCachedFlag",
                        FeatureMap.class,
                        String.class,
                        boolean.class,
                        boolean.class));

        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/SwipeRefreshHandler",
                        "org/chromium/chrome/browser/BraveSwipeRefreshHandler",
                        Tab.class));

        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/logo/LogoMediator",
                        "org/chromium/chrome/browser/logo/BraveLogoMediator",
                        Context.class,
                        Callback.class,
                        PropertyModel.class,
                        Callback.class,
                        LogoCoordinator.VisibilityObserver.class,
                        CachedTintedBitmap.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/notifications/permissions/NotificationPermissionRationaleDialogController",
                        "org/chromium/chrome/browser/notifications/permissions/BraveNotificationPermissionRationaleDialogController",
                        Context.class,
                        ModalDialogManager.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/notifications/StandardNotificationBuilder",
                        "org/chromium/chrome/browser/notifications/BraveNotificationBuilder",
                        Context.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator",
                        "org/chromium/chrome/browser/tabbed_mode/BraveTabbedRootUiCoordinator",
                        AppCompatActivity.class,
                        Callback.class,
                        ObservableSupplier.class,
                        ActivityTabProvider.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        ObservableSupplier.class,
                        OneshotSupplier.class,
                        OneshotSupplier.class,
                        OneshotSupplier.class,
                        OneshotSupplier.class,
                        OneshotSupplier.class,
                        Supplier.class,
                        BrowserControlsManager.class,
                        ActivityWindowAndroid.class,
                        ActivityLifecycleDispatcher.class,
                        ObservableSupplier.class,
                        MenuOrKeyboardActionController.class,
                        Supplier.class,
                        ObservableSupplier.class,
                        AppMenuBlocker.class,
                        BooleanSupplier.class,
                        BooleanSupplier.class,
                        Supplier.class,
                        FullscreenManager.class,
                        Supplier.class,
                        Supplier.class,
                        Supplier.class,
                        ObservableSupplierImpl.class,
                        int.class,
                        Supplier.class,
                        AppMenuDelegate.class,
                        StatusBarColorProvider.class,
                        OneshotSupplierImpl.class,
                        IntentRequestTracker.class,
                        InsetObserver.class,
                        Function.class,
                        Callback.class,
                        boolean.class,
                        BackPressManager.class,
                        Bundle.class,
                        MultiInstanceManager.class,
                        ObservableSupplier.class,
                        View.class,
                        ManualFillingComponentSupplier.class,
                        EdgeToEdgeStateProvider.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkToolbar",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkToolbar",
                        Context.class,
                        AttributeSet.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkToolbarCoordinator",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkToolbarCoordinator",
                        Context.class,
                        SelectableListLayout.class,
                        SelectionDelegate.class,
                        SearchDelegate.class,
                        DragReorderableRecyclerViewAdapter.class,
                        boolean.class,
                        OneshotSupplier.class,
                        BookmarkModel.class,
                        BookmarkOpener.class,
                        BookmarkUiPrefs.class,
                        ModalDialogManager.class,
                        Runnable.class,
                        BookmarkMoveSnackbarManager.class,
                        BooleanSupplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkManagerCoordinator",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkManagerCoordinator",
                        Context.class,
                        ComponentName.class,
                        boolean.class,
                        SnackbarManager.class,
                        Profile.class,
                        BookmarkUiPrefs.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkManagerMediator",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkManagerMediator",
                        Context.class,
                        BookmarkModel.class,
                        BookmarkOpener.class,
                        SelectableListLayout.class,
                        SelectionDelegate.class,
                        RecyclerView.class,
                        DragReorderableRecyclerViewAdapter.class,
                        boolean.class,
                        ObservableSupplierImpl.class,
                        Profile.class,
                        BookmarkUndoController.class,
                        ModelList.class,
                        BookmarkUiPrefs.class,
                        Runnable.class,
                        BookmarkImageFetcher.class,
                        ShoppingService.class,
                        SnackbarManager.class,
                        Consumer.class,
                        BookmarkMoveSnackbarManager.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "org/chromium/chrome/browser/bookmarks/BraveBookmarkBridge", long.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkModel",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkModel",
                        long.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkPage",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkPage",
                        ComponentName.class,
                        SnackbarManager.class,
                        Profile.class,
                        NativePageHost.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/feedback/HelpAndFeedbackLauncherImpl",
                        "org/chromium/chrome/browser/feedback/BraveHelpAndFeedbackLauncherImpl",
                        Profile.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/firstrun/FreIntentCreator",
                        "org/chromium/chrome/browser/firstrun/BraveFreIntentCreator"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/components/external_intents/ExternalNavigationHandler",
                        "org/chromium/chrome/browser/externalnav/BraveExternalNavigationHandler",
                        ExternalNavigationDelegate.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/contextmenu/ChromeContextMenuPopulator",
                        "org/chromium/chrome/browser/contextmenu/BraveChromeContextMenuPopulator",
                        TabContextMenuItemDelegate.class,
                        Supplier.class,
                        int.class,
                        ExternalAuthUtils.class,
                        Context.class,
                        ContextMenuParams.class,
                        ContextMenuNativeDelegate.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/base/shared_preferences/StrictPreferenceKeyChecker",
                        "org/chromium/base/shared_preferences/BraveStrictPreferenceKeyChecker",
                        PreferenceKeyRegistry.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/identity_disc/IdentityDiscController",
                        "org/chromium/chrome/browser/identity_disc/BraveIdentityDiscController",
                        Context.class,
                        ActivityLifecycleDispatcher.class,
                        ObservableSupplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/bookmarks/BookmarkUiPrefs",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkUiPrefs",
                        SharedPreferencesManager.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/customtabs/features/partialcustomtab/PartialCustomTabBottomSheetStrategy", // presubmit: ignore-long-line
                        "org/chromium/chrome/browser/customtabs/features/partialcustomtab/BravePartialCustomTabBottomSheetStrategy", // presubmit: ignore-long-line
                        Activity.class,
                        BrowserServicesIntentDataProvider.class,
                        Supplier.class,
                        Supplier.class,
                        BravePartialCustomTabBottomSheetStrategy.getOnResizedCallbackClass(),
                        BravePartialCustomTabBottomSheetStrategy.getOnActivityLayoutCallbackClass(),
                        ActivityLifecycleDispatcher.class,
                        FullscreenManager.class,
                        boolean.class,
                        boolean.class,
                        PartialCustomTabHandleStrategyFactory.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/compositor/layouts/ToolbarSwipeLayout",
                        "org/chromium/chrome/browser/compositor/layouts/BraveToolbarSwipeLayout",
                        Context.class,
                        LayoutUpdateHost.class,
                        LayoutRenderHost.class,
                        BrowserControlsStateProvider.class,
                        LayoutManager.class,
                        TopUiThemeColorProvider.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/components/embedder_support/view/ContentView",
                        "org/chromium/components/embedder_support/view/BraveContentView",
                        Context.class,
                        WebContents.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/components/signin/SystemAccountManagerDelegate",
                        "org/chromium/components/signin/BraveSystemAccountManagerDelegate"));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/AutocompleteEditText",
                        "org/chromium/chrome/browser/omnibox/BraveAutocompleteEditText",
                        Context.class,
                        AttributeSet.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor",
                        "org/chromium/chrome/browser/omnibox/suggestions/editurl/BraveEditUrlSuggestionProcessor",
                        Context.class,
                        SuggestionHost.class,
                        Optional.class,
                        Supplier.class,
                        Supplier.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/password_manager/settings/ExportFlow",
                        "org/chromium/chrome/browser/password_manager/settings/BraveExportFlow",
                        int.class));

        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/ui/system/StatusBarColorController",
                        "org/chromium/chrome/browser/ui/system/BraveStatusBarColorController",
                        Window.class,
                        boolean.class,
                        Context.class,
                        StatusBarColorProvider.class,
                        ObservableSupplier.class,
                        ActivityLifecycleDispatcher.class,
                        ActivityTabProvider.class,
                        TopUiThemeColorProvider.class));
        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/browsing_data/ClearBrowsingDataFragmentAdvanced", // presubmit: ignore-long-line
                        "org/chromium/chrome/browser/browsing_data/BraveClearBrowsingDataFragmentAdvanced")); // presubmit: ignore-long-line

        Assert.assertTrue(
                constructorsMatch(
                        "org/chromium/chrome/browser/tasks/tab_management/IncognitoTabSwitcherPane",
                        "org/chromium/chrome/browser/tasks/tab_management/BraveIncognitoTabSwitcherPane", // presubmit: ignore-long-line
                        Context.class,
                        OneshotSupplier.class,
                        TabSwitcherPaneCoordinatorFactory.class,
                        Supplier.class,
                        OnClickListener.class,
                        OneshotSupplier.class,
                        DoubleConsumer.class,
                        UserEducationHelper.class,
                        ObservableSupplier.class));
    }

    @Test
    @SmallTest
    public void testFieldsExist() throws Exception {
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
                fieldExists("org/chromium/chrome/browser/ChromeTabbedActivity", "mLayoutManager"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mNewTabPageLayout"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mFeedSurfaceProvider"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mToolbarSupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/ntp/NewTabPage", "mBottomSheetController"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/ntp/NewTabPage", "mTabStripHeightSupplier"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/suggestions/tile/MostVisitedTilesMediator",
                        "mTileGroup"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/sync/settings/ManageSyncSettings",
                        "mGoogleActivityControls"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/settings/ManageSyncSettings", "mSyncEncryption"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/sync/settings/ManageSyncSettings", "mSyncEverything"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/password_manager/settings/PasswordAccessReauthenticationHelper", // presubmit: ignore-long-line
                        "mCallback"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/password_manager/settings/PasswordAccessReauthenticationHelper", // presubmit: ignore-long-line
                        "mFragmentManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/bottom/BottomControlsCoordinator",
                        "mMediator"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mBottomControlsCoordinatorSupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mReadAloudControllerSupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mCallbackController"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mBottomControlsStacker"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mFullscreenManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mActivityTabProvider"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mAppThemeColorProvider"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager", "mScrimCoordinator"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mMenuButtonCoordinator"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mToolbarTabController"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager", "mLocationBar"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mActionModeController"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager", "mLocationBarModel"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/toolbar/ToolbarManager", "mToolbar"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mBookmarkModelSupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager", "mLayoutManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mOverlayPanelVisibilitySupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager", "mTabModelSelector"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mIncognitoStateProvider"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mBottomSheetController"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mTabContentManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mTabCreatorManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mModalDialogManagerSupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/ToolbarManager",
                        "mTabObscuringHandler"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "mOptionalButtonController"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator",
                        "mToolbarColorObserverManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/app/ChromeActivity",
                        "mBrowserControlsManagerSupplier"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                        "mBottomControlsHeight"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                        "mModel"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/bottom/BottomControlsMediator",
                        "mBottomControlsStacker"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator",
                        "mToolbarView"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                        "mProfile"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings",
                        "mProfile"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings",
                        "mSearchEngineAdapter"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "mCategory",
                        true,
                        SiteSettingsCategory.class));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "mSite",
                        true,
                        Website.class));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/components/variations/firstrun/VariationsSeedFetcher",
                        "sLock",
                        true,
                        Object.class));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/components/variations/firstrun/VariationsSeedFetcher",
                        "DEFAULT_VARIATIONS_SERVER_URL",
                        true,
                        String.class));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/components/variations/firstrun/VariationsSeedFetcher",
                        "DEFAULT_FAST_VARIATIONS_SERVER_URL",
                        true,
                        String.class));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "mNativeInitialized",
                        true,
                        boolean.class));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "mDropdownViewInfoListManager"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "mDropdownViewInfoListBuilder"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "mDataProvider"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "mContext"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/multiwindow/MultiInstanceManagerApi31",
                        "mInstanceId"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout",
                        "mMvTilesContainerLayout"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout", "mLogoCoordinator"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/ntp/NewTabPageLayout", "mInitialTileNum"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/dom_distiller/ReaderModeManager", "mTab"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarCoordinator",
                        "mLocationBarMediator"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarCoordinator", "mUrlBar"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "mNativeInitialized"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "mWindowAndroid"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "mLocationBarLayout"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "mIsUrlFocusChangeInProgress"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator", "mUrlHasFocus"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator", "mIsTablet"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "mIsLocationBarFocusedFromNtpScroll"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/omnibox/LocationBarMediator", "mContext"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/omnibox/LocationBarMediator",
                        "mBrandedColorScheme"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/logo/LogoMediator", "mLogoModel"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/logo/LogoMediator", "mShouldShowLogo"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/app/bookmarks/BookmarkActivity",
                "mBookmarkManagerCoordinator"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/bookmarks/BookmarkManagerCoordinator", "mMediator"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/bookmarks/BookmarkManagerMediator", "mBookmarkModel"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/bookmarks/BookmarkManagerMediator", "mContext"));
        Assert.assertTrue(fieldExists(
                "org/chromium/chrome/browser/bookmarks/BookmarkToolbarCoordinator", "mToolbar"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/bookmarks/BookmarkPage",
                        "mBookmarkManagerCoordinator"));
        Assert.assertTrue(
                fieldExists("org/chromium/components/cached_flags/CachedFlag", "mDefaultValue"));
        Assert.assertFalse(
                fieldExists(
                        "org/chromium/chrome/browser/tabmodel/TabGroupModelFilterImpl",
                        "mIsResetting"));
        Assert.assertTrue(
                fieldExists("org/chromium/chrome/browser/SwipeRefreshHandler", "mSwipeType"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/SwipeRefreshHandler", "mTab"));

        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/customtabs/features/partialcustomtab/PartialCustomTabBottomSheetStrategy", // presubmit: ignore-long-line
                        "mStopShowingSpinner"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/customtabs/features/partialcustomtab/PartialCustomTabBottomSheetStrategy", // presubmit: ignore-long-line
                        "mTab"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/compositor/layouts/ToolbarSwipeLayout",
                        "mMoveToolbar"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/top/ToolbarPhone",
                        "mLocationBarBackgroundColorForNtp"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/toolbar/top/ToolbarPhone",
                        "mToolbarBackgroundColorForNtp"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/ui/system/StatusBarColorController",
                        "mBackgroundColorForNtp"));
        Assert.assertTrue(
                fieldExists(
                        "org/chromium/chrome/browser/tasks/tab_management/IncognitoTabSwitcherPane",
                        "mReferenceButtonData",
                        true,
                        ResourceButtonData.class));
        Assert.assertFalse(
                fieldExists(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedNavigationBarColorController", // presubmit: ignore-long-line
                        "mContext"));
        Assert.assertFalse(
                fieldExists(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedNavigationBarColorController", // presubmit: ignore-long-line
                        "mTabModelSelector"));
    }

    @Test
    @SmallTest
    public void testSuperNames() throws Exception {
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/ChromeApplicationImpl",
                        "org/chromium/chrome/browser/BraveApplicationImplBase"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/settings/MainSettings",
                        "org/chromium/chrome/browser/settings/BraveMainPreferencesBase"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/ntp/NewTabPageLayout",
                        "android/widget/FrameLayout"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/password_manager/settings/PasswordSettings",
                        "org/chromium/chrome/browser/password_manager/settings/BravePasswordSettingsBase")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                        "org/chromium/chrome/browser/search_engines/settings/BraveBaseSearchEngineAdapter")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "org/chromium/components/browser_ui/site_settings/BraveSingleCategorySettings")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/ChromeTabbedActivity",
                        "org/chromium/chrome/browser/app/BraveActivity"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate",
                        "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/customtabs/CustomTabAppMenuPropertiesDelegate",
                        "org/chromium/chrome/browser/app/appmenu/BraveAppMenuPropertiesDelegateImpl")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/suggestions/tile/SuggestionsTileView",
                        "org/chromium/chrome/browser/suggestions/tile/BraveTileView"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/customtabs/features/toolbar/CustomTabToolbar",
                        "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/toolbar/top/ToolbarPhone",
                        "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/toolbar/top/ToolbarTablet",
                        "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/components/browser_ui/site_settings/SingleCategorySettings",
                        "org/chromium/components/browser_ui/site_settings/BraveSingleCategorySettings")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings",
                        "org/chromium/components/browser_ui/site_settings/BraveSingleWebsiteSettings")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/components/browser_ui/site_settings/Website",
                        "org/chromium/components/browser_ui/site_settings/BraveWebsite"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/components/browser_ui/site_settings/SiteSettings",
                        "org/chromium/components/browser_ui/site_settings/BraveSiteSettingsPreferencesBase"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/download/DownloadMessageUiControllerImpl",
                        "org/chromium/chrome/browser/download/BraveDownloadMessageUiControllerImpl")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteCoordinator",
                        "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteCoordinator")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator",
                        "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediatorBase")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/omnibox/LocationBarPhone",
                        "org/chromium/chrome/browser/omnibox/BraveLocationBarLayout"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/omnibox/LocationBarTablet",
                        "org/chromium/chrome/browser/omnibox/BraveLocationBarLayout"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/searchwidget/SearchActivityLocationBarLayout",
                        "org/chromium/chrome/browser/omnibox/BraveLocationBarLayout"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/document/ChromeLauncherActivity",
                        "org/chromium/chrome/browser/document/BraveLauncherActivity"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/components/permissions/PermissionDialogDelegate",
                        "org/chromium/components/permissions/BravePermissionDialogDelegate"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/tracing/settings/DeveloperSettings",
                        "org/chromium/chrome/browser/settings/BravePreferenceFragment"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/bookmarks/BookmarkModel",
                        "org/chromium/chrome/browser/bookmarks/BraveBookmarkBridge"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/tabmodel/TabGroupModelFilterImpl",
                        "org/chromium/chrome/browser/tabmodel/BraveTabGroupModelFilter"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/media/PictureInPictureActivity",
                        "org/chromium/chrome/browser/media/BravePictureInPictureActivity"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/suggestions/tile/MostVisitedTilesLayout",
                        "org/chromium/chrome/browser/suggestions/tile/BraveMostVisitedTilesLayoutBase"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor",
                        "org/chromium/chrome/browser/omnibox/suggestions/editurl/BraveEditUrlSuggestionProcessorBase"));
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/tasks/tab_management/TabSwitcherPane",
                        "org/chromium/chrome/browser/tasks/tab_management/BraveTabSwitcherPaneBase")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/tasks/tab_management/IncognitoTabSwitcherPane",
                        "org/chromium/chrome/browser/tasks/tab_management/BraveTabSwitcherPaneBase")); // presubmit: ignore-long-line
        Assert.assertTrue(
                checkSuperName(
                        "org/chromium/chrome/browser/tabbed_mode/TabbedNavigationBarColorController", // presubmit: ignore-long-line
                        "org/chromium/chrome/browser/tabbed_mode/BraveTabbedNavigationBarColorControllerBase")); // presubmit: ignore-long-line
    }

    private boolean classExists(String className) {
        return getClassForPath(className) != null;
    }

    private boolean methodExists(
            String className,
            String methodName,
            MethodModifier methodModifier,
            Boolean checkTypes,
            Class<?> returnType,
            Class<?>... parameterTypes) {
        Class c = getClassForPath(className);
        if (c == null) {
            return false;
        }
        for (Method m : c.getDeclaredMethods()) {
            if (m.getName().equals(methodName)) {
                if (checkTypes) {
                    Class<?> type = m.getReturnType();
                    if ((type == null && returnType != null)
                            || (type != null && returnType == null)
                            || (type != null && returnType != null && !type.equals(returnType))) {
                        return false;
                    }
                    Class<?>[] types = m.getParameterTypes();
                    if ((types == null && parameterTypes != null)
                            || (types != null && parameterTypes == null)
                            || (types != null
                                    && parameterTypes != null
                                    && types.length != parameterTypes.length)) {
                        return false;
                    }
                    for (int i = 0; i < (types == null ? 0 : types.length); i++) {
                        if (!types[i].equals(parameterTypes[i])) {
                            return false;
                        }
                    }
                }

                if (methodModifier == MethodModifier.STATIC
                        && !Modifier.isStatic(m.getModifiers())) {
                    return false;
                }

                if (methodModifier == MethodModifier.REGULAR
                        && Modifier.isStatic(m.getModifiers())) {
                    return false;
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
                } else {
                    return true;
                }
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

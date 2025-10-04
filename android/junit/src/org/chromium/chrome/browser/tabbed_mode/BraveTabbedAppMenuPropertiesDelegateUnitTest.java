/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import static androidx.test.espresso.matcher.ViewMatchers.assertThat;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.ContextThemeWrapper;
import android.view.View;

import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowPackageManager;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.test.BaseRobolectricTestRule;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.appmenu.AppMenuPropertiesDelegateImpl.MenuGroup;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.PowerBookmarkUtils;
import org.chromium.chrome.browser.commerce.ShoppingServiceFactory;
import org.chromium.chrome.browser.enterprise.util.ManagedBrowserUtils;
import org.chromium.chrome.browser.enterprise.util.ManagedBrowserUtilsJni;
import org.chromium.chrome.browser.feed.FeedFeatures;
import org.chromium.chrome.browser.feed.webfeed.WebFeedBridge;
import org.chromium.chrome.browser.feed.webfeed.WebFeedBridgeJni;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.incognito.IncognitoUtilsJni;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.offlinepages.OfflinePageUtils;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.readaloud.ReadAloudController;
import org.chromium.chrome.browser.signin.services.IdentityServicesProvider;
import org.chromium.chrome.browser.signin.services.SigninManager;
import org.chromium.chrome.browser.sync.SyncServiceFactory;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabGroupModelFilter;
import org.chromium.chrome.browser.tabmodel.TabGroupModelFilterProvider;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.translate.TranslateBridge;
import org.chromium.chrome.browser.translate.TranslateBridgeJni;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePage;
import org.chromium.components.browser_ui.accessibility.PageZoomManager;
import org.chromium.components.browser_ui.accessibility.PageZoomUtils;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridge;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridgeJni;
import org.chromium.components.commerce.core.CommerceFeatureUtils;
import org.chromium.components.commerce.core.CommerceFeatureUtilsJni;
import org.chromium.components.commerce.core.ShoppingService;
import org.chromium.components.content_settings.ContentSetting;
import org.chromium.components.dom_distiller.core.DomDistillerFeatures;
import org.chromium.components.power_bookmarks.PowerBookmarkMeta;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.signin.identitymanager.ConsentLevel;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.sync.SyncService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.user_prefs.UserPrefsJni;
import org.chromium.components.webapps.AppBannerManager;
import org.chromium.components.webapps.AppBannerManagerJni;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.WebContents;
import org.chromium.google_apis.gaia.GoogleServiceAuthError;
import org.chromium.google_apis.gaia.GoogleServiceAuthErrorState;
import org.chromium.ui.accessibility.AccessibilityState;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.url.JUnitTestGURLs;

import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link BraveTabbedAppMenuPropertiesDelegate}. */
@RunWith(BaseRobolectricTestRunner.class)
@DisableFeatures({
    BraveFeatureList.BRAVE_PLAYLIST,
    ChromeFeatureList.ADAPTIVE_BUTTON_IN_TOP_TOOLBAR_PAGE_SUMMARY,
    ChromeFeatureList.FEED_AUDIO_OVERVIEWS,
    ChromeFeatureList.NEW_TAB_PAGE_CUSTOMIZATION,
    ChromeFeatureList.PROPAGATE_DEVICE_CONTENT_FILTERS_TO_SUPERVISED_USER,
    DomDistillerFeatures.READER_MODE_IMPROVEMENTS,
})
@EnableFeatures({
    BraveFeatureList.AI_CHAT,
    BraveFeatureList.NATIVE_BRAVE_WALLET,
})
public class BraveTabbedAppMenuPropertiesDelegateUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private ActivityTabProvider mActivityTabProvider;
    @Mock private Tab mTab;
    @Mock private WebContents mWebContents;
    @Mock private NativePage mNativePage;
    @Mock private NavigationController mNavigationController;
    @Mock private MultiWindowModeStateDispatcher mMultiWindowModeStateDispatcher;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private TabModel mTabModel;
    @Mock private TabModel mIncognitoTabModel;
    @Mock private ToolbarManager mToolbarManager;
    @Mock private View mDecorView;
    @Mock private LayoutStateProvider mLayoutStateProvider;
    @Mock private ManagedBrowserUtils.Natives mManagedBrowserUtilsJniMock;
    @Mock private Profile mProfile;
    @Mock private AppMenuDelegate mAppMenuDelegate;
    @Mock private WebFeedSnackbarController.FeedLauncher mFeedLauncher;
    @Mock private ModalDialogManager mDialogManager;
    @Mock private SnackbarManager mSnackbarManager;
    @Mock private OfflinePageUtils.Internal mOfflinePageUtils;
    @Mock private SigninManager mSigninManager;
    @Mock private IdentityManager mIdentityManager;
    @Mock private IdentityServicesProvider mIdentityService;
    @Mock private TabGroupModelFilterProvider mTabGroupModelFilterProvider;
    @Mock private TabGroupModelFilter mTabGroupModelFilter;
    @Mock private IncognitoUtils.Natives mIncognitoUtilsJniMock;
    @Mock public WebsitePreferenceBridge.Natives mWebsitePreferenceBridgeJniMock;
    @Mock private IncognitoReauthController mIncognitoReauthControllerMock;
    @Mock private CommerceFeatureUtils.Natives mCommerceFeatureUtilsJniMock;
    @Mock private ShoppingService mShoppingService;
    @Mock private AppBannerManager.Natives mAppBannerManagerJniMock;
    @Mock private ReadAloudController mReadAloudController;
    @Mock private UserPrefs.Natives mUserPrefsNatives;
    @Mock private PrefService mPrefService;
    @Mock private SyncService mSyncService;
    @Mock private WebFeedBridge.Natives mWebFeedBridgeJniMock;
    @Mock private TranslateBridge.Natives mTranslateBridgeJniMock;
    @Mock private PageZoomManager mPageZoomManagerMock;

    private ShadowPackageManager mShadowPackageManager;

    private final OneshotSupplierImpl<LayoutStateProvider> mLayoutStateProviderSupplier =
            new OneshotSupplierImpl<>();
    private final OneshotSupplierImpl<IncognitoReauthController>
            mIncognitoReauthControllerSupplier = new OneshotSupplierImpl<>();
    private final ObservableSupplierImpl<BookmarkModel> mBookmarkModelSupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplierImpl<ReadAloudController> mReadAloudControllerSupplier =
            new ObservableSupplierImpl<>();

    private BraveTabbedAppMenuPropertiesDelegate mTabbedAppMenuPropertiesDelegate;

    // Boolean flags to test multi-window menu visibility for various combinations.
    private boolean mIsMultiInstance;
    private boolean mIsMultiWindow;
    private boolean mIsTabletScreen;
    private boolean mIsMultiWindowApiSupported;
    private boolean mIsMoveToOtherWindowSupported;

    // Used to ensure all the combinations are tested.
    private final boolean[] mFlagCombinations = new boolean[1 << 5];

    @Before
    public void setUp() {
        Context context =
                new ContextThemeWrapper(
                        ContextUtils.getApplicationContext(), R.style.Theme_BrowserUI_DayNight);

        mShadowPackageManager = Shadows.shadowOf(context.getPackageManager());

        mLayoutStateProviderSupplier.set(mLayoutStateProvider);
        mIncognitoReauthControllerSupplier.set(mIncognitoReauthControllerMock);
        mReadAloudControllerSupplier.set(mReadAloudController);
        when(mTab.getWebContents()).thenReturn(mWebContents);
        when(mTab.getProfile()).thenReturn(mProfile);
        when(mWebContents.getNavigationController()).thenReturn(mNavigationController);
        when(mNavigationController.getUseDesktopUserAgent()).thenReturn(false);
        when(mTabModelSelector.isTabStateInitialized()).thenReturn(true);
        when(mTabModelSelector.getCurrentModel()).thenReturn(mTabModel);
        when(mTabModelSelector.getModel(false)).thenReturn(mTabModel);
        when(mTabModelSelector.getModel(true)).thenReturn(mIncognitoTabModel);
        when(mTabModel.isIncognito()).thenReturn(false);
        when(mIncognitoTabModel.isIncognito()).thenReturn(true);
        when(mTabModelSelector.getTabGroupModelFilterProvider())
                .thenReturn(mTabGroupModelFilterProvider);
        when(mTabGroupModelFilterProvider.getCurrentTabGroupModelFilter())
                .thenReturn(mTabGroupModelFilter);
        when(mTabGroupModelFilter.getTabModel()).thenReturn(mTabModel);
        when(mTabModel.getProfile()).thenReturn(mProfile);
        ManagedBrowserUtilsJni.setInstanceForTesting(mManagedBrowserUtilsJniMock);
        ProfileManager.setLastUsedProfileForTesting(mProfile);
        WebsitePreferenceBridgeJni.setInstanceForTesting(mWebsitePreferenceBridgeJniMock);
        OfflinePageUtils.setInstanceForTesting(mOfflinePageUtils);
        when(mIdentityService.getSigninManager(any(Profile.class))).thenReturn(mSigninManager);
        when(mSigninManager.getIdentityManager()).thenReturn(mIdentityManager);
        IdentityServicesProvider.setInstanceForTests(mIdentityService);
        when(mIdentityService.getIdentityManager(any(Profile.class))).thenReturn(mIdentityManager);
        when(mIdentityManager.hasPrimaryAccount(ConsentLevel.SIGNIN)).thenReturn(true);

        PageZoomUtils.setShouldShowMenuItemForTesting(false);
        FeedFeatures.setFakePrefsForTest(mPrefService);
        AppBannerManagerJni.setInstanceForTesting(mAppBannerManagerJniMock);
        Mockito.when(mAppBannerManagerJniMock.getInstallableWebAppManifestId(any()))
                .thenReturn(null);
        WebFeedBridgeJni.setInstanceForTesting(mWebFeedBridgeJniMock);
        when(mWebFeedBridgeJniMock.isWebFeedEnabled()).thenReturn(true);
        UserPrefsJni.setInstanceForTesting(mUserPrefsNatives);
        when(mUserPrefsNatives.get(mProfile)).thenReturn(mPrefService);

        when(mSyncService.getAuthError())
                .thenReturn(new GoogleServiceAuthError(GoogleServiceAuthErrorState.NONE));
        when(mSyncService.hasUnrecoverableError()).thenReturn(false);
        when(mSyncService.isEngineInitialized()).thenReturn(true);
        when(mSyncService.isPassphraseRequiredForPreferredDataTypes()).thenReturn(false);
        when(mSyncService.isTrustedVaultKeyRequiredForPreferredDataTypes()).thenReturn(false);
        when(mSyncService.isTrustedVaultRecoverabilityDegraded()).thenReturn(false);

        SyncServiceFactory.setInstanceForTesting(mSyncService);

        IncognitoUtilsJni.setInstanceForTesting(mIncognitoUtilsJniMock);

        TranslateBridgeJni.setInstanceForTesting(mTranslateBridgeJniMock);
        Mockito.when(mTranslateBridgeJniMock.canManuallyTranslate(any(), anyBoolean()))
                .thenReturn(false);

        PowerBookmarkUtils.setPriceTrackingEligibleForTesting(false);
        PowerBookmarkUtils.setPowerBookmarkMetaForTesting(PowerBookmarkMeta.newBuilder().build());
        BraveTabbedAppMenuPropertiesDelegate delegate =
                new BraveTabbedAppMenuPropertiesDelegate(
                        context,
                        mActivityTabProvider,
                        mMultiWindowModeStateDispatcher,
                        mTabModelSelector,
                        mToolbarManager,
                        mDecorView,
                        mAppMenuDelegate,
                        mLayoutStateProviderSupplier,
                        mBookmarkModelSupplier,
                        mFeedLauncher,
                        mDialogManager,
                        mSnackbarManager,
                        mIncognitoReauthControllerSupplier,
                        mReadAloudControllerSupplier,
                        mPageZoomManagerMock);
        delegate.setIsJunitTesting(true);
        BaseRobolectricTestRule.runAllBackgroundAndUi();
        mTabbedAppMenuPropertiesDelegate = Mockito.spy(delegate);

        ChromeSharedPreferences.getInstance()
                .removeKeysWithPrefix(ChromePreferenceKeys.MULTI_INSTANCE_URL);
        ChromeSharedPreferences.getInstance()
                .removeKeysWithPrefix(ChromePreferenceKeys.MULTI_INSTANCE_TAB_COUNT);

        CommerceFeatureUtilsJni.setInstanceForTesting(mCommerceFeatureUtilsJniMock);
        ShoppingServiceFactory.setShoppingServiceForTesting(mShoppingService);
    }

    @After
    public void tearDown() {
        AccessibilityState.setIsKnownScreenReaderEnabledForTesting(false);
    }

    private void assertMenuItemsAreEqual(
            MVCListAdapter.ModelList modelList, Integer... expectedItems) {
        List<Integer> actualItems = new ArrayList<>();
        for (MVCListAdapter.ListItem item : modelList) {
            actualItems.add(item.model.get(AppMenuItemProperties.MENU_ITEM_ID));
        }

        assertThat(
                "Populated menu items were:" + getMenuTitles(modelList),
                actualItems,
                Matchers.containsInAnyOrder(expectedItems));
    }

    private void setUpIncognitoMocks() {
        doReturn(true).when(mTabbedAppMenuPropertiesDelegate).isIncognitoEnabled();
        doReturn(false).when(mIncognitoReauthControllerMock).isIncognitoReauthPending();
        doReturn(false).when(mIncognitoReauthControllerMock).isReauthPageShowing();
    }

    @Test
    @Config(qualifiers = "sw320dp")
    public void testBravePageMenuItems_Ntp() {
        setUpMocksForPageMenu();
        when(mTab.getUrl()).thenReturn(JUnitTestGURLs.NTP_URL);
        when(mTab.isNativePage()).thenReturn(true);
        when(mNativePage.isPdf()).thenReturn(false);
        when(mTab.getNativePage()).thenReturn(mNativePage);
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowTranslateMenuItem(any(Tab.class));

        assertEquals(MenuGroup.PAGE_MENU, mTabbedAppMenuPropertiesDelegate.getMenuGroup());
        MVCListAdapter.ModelList modelList = mTabbedAppMenuPropertiesDelegate.getMenuItems();

        Integer[] expectedItems = {
            R.id.new_tab_menu_id,
            R.id.new_incognito_tab_menu_id,
            R.id.add_to_group_menu_id,
            R.id.divider_line_id,
            R.id.open_history_menu_id,
            R.id.downloads_menu_id,
            R.id.all_bookmarks_menu_id,
            R.id.brave_wallet_id,
            R.id.brave_leo_id,
            R.id.recent_tabs_menu_id,
            R.id.divider_line_id,
            R.id.preferences_id,
            R.id.set_default_browser,
            R.id.brave_news_id,
            R.id.brave_customize_menu_id,
            R.id.exit_id,
        };
        assertMenuItemsAreEqual(modelList, expectedItems);
    }

    @Test
    @Config(qualifiers = "sw320dp")
    public void testBravePageMenuItems_RegularPage() {
        setUpMocksForPageMenu();
        setMenuOptions(
                new MenuOptions()
                        .withShowTranslate()
                        .withShowAddToHomeScreen()
                        .withAutoDarkEnabled());

        assertEquals(MenuGroup.PAGE_MENU, mTabbedAppMenuPropertiesDelegate.getMenuGroup());
        MVCListAdapter.ModelList modelList = mTabbedAppMenuPropertiesDelegate.getMenuItems();

        List<Integer> expectedItems = new ArrayList<>();

        expectedItems.add(R.id.new_tab_menu_id);
        expectedItems.add(R.id.new_incognito_tab_menu_id);
        expectedItems.add(R.id.add_to_group_menu_id);
        expectedItems.add(R.id.divider_line_id);
        expectedItems.add(R.id.open_history_menu_id);
        expectedItems.add(R.id.downloads_menu_id);
        expectedItems.add(R.id.all_bookmarks_menu_id);
        expectedItems.add(R.id.brave_wallet_id);
        expectedItems.add(R.id.brave_leo_id);
        expectedItems.add(R.id.recent_tabs_menu_id);
        expectedItems.add(R.id.divider_line_id);
        expectedItems.add(R.id.find_in_page_id);
        expectedItems.add(R.id.translate_id);
        expectedItems.add(R.id.universal_install);
        expectedItems.add(R.id.request_desktop_site_id);
        expectedItems.add(R.id.auto_dark_web_contents_id);
        expectedItems.add(R.id.divider_line_id);
        expectedItems.add(R.id.set_default_browser);
        expectedItems.add(R.id.preferences_id);
        expectedItems.add(R.id.brave_news_id);
        expectedItems.add(R.id.brave_customize_menu_id);
        expectedItems.add(R.id.exit_id);

        assertMenuItemsAreEqual(modelList, expectedItems.toArray(new Integer[0]));
    }

    private void setUpMocksForPageMenu() {
        when(mActivityTabProvider.get()).thenReturn(mTab);
        when(mLayoutStateProvider.isLayoutVisible(LayoutType.TAB_SWITCHER)).thenReturn(false);
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldCheckBookmarkStar(any(Tab.class));
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldEnableDownloadPage(any(Tab.class));
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowReaderModePrefs(any(Tab.class));
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowManagedByMenuItem(any(Tab.class));
        doReturn(true)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowAutoDarkItem(any(Tab.class), eq(false));
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowAutoDarkItem(any(Tab.class), eq(true));
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowContentFilterHelpCenterMenuItem(any(Tab.class));

        setUpIncognitoMocks();
    }

    private String getMenuTitles(MVCListAdapter.ModelList modelList) {
        StringBuilder items = new StringBuilder();
        for (MVCListAdapter.ListItem item : modelList) {
            CharSequence title =
                    item.model.containsKey(AppMenuItemProperties.TITLE)
                            ? item.model.get(AppMenuItemProperties.TITLE)
                            : null;
            if (title == null) {
                if (item.type == AppMenuHandler.AppMenuItemType.BUTTON_ROW) {
                    title = "Icon Row";
                } else if (item.type == AppMenuHandler.AppMenuItemType.DIVIDER) {
                    title = "Divider";
                }
            }
            items.append("\n")
                    .append(title)
                    .append(":")
                    .append(item.model.get(AppMenuItemProperties.MENU_ITEM_ID));
        }
        return items.toString();
    }

    /** Options for tests that control how Menu is being rendered. */
    // Suppressed warnings for withShowUpdate / withShowPaintPreview / withNativePage /
    // withShowReaderModePrefs as we may want to use it when will expand the tests
    @SuppressWarnings("UnusedMethod")
    private static class MenuOptions {
        private boolean mIsNativePage;
        private boolean mShowTranslate;
        private boolean mShowUpdate;
        private boolean mShowMoveToOtherWindow;
        private boolean mShowReaderModePrefs;
        private boolean mShowAddToHomeScreen;
        private boolean mShowPaintPreview;
        private boolean mIsAutoDarkEnabled;

        protected boolean isNativePage() {
            return mIsNativePage;
        }

        protected boolean showTranslate() {
            return mShowTranslate;
        }

        protected boolean showUpdate() {
            return mShowUpdate;
        }

        protected boolean showMoveToOtherWindow() {
            return mShowMoveToOtherWindow;
        }

        protected boolean showReaderModePrefs() {
            return mShowReaderModePrefs;
        }

        protected boolean showPaintPreview() {
            return mShowPaintPreview;
        }

        protected boolean isAutoDarkEnabled() {
            return mIsAutoDarkEnabled;
        }

        protected MenuOptions setShowTranslate(boolean state) {
            mShowTranslate = state;
            return this;
        }

        protected MenuOptions setShowAddToHomeScreen(boolean state) {
            mShowAddToHomeScreen = state;
            return this;
        }

        protected MenuOptions setAutoDarkEnabled(boolean state) {
            mIsAutoDarkEnabled = state;
            return this;
        }

        protected MenuOptions withShowTranslate() {
            return setShowTranslate(true);
        }

        protected MenuOptions withShowAddToHomeScreen() {
            return setShowAddToHomeScreen(true);
        }

        protected MenuOptions withAutoDarkEnabled() {
            return setAutoDarkEnabled(true);
        }
    }

    private void setMenuOptions(MenuOptions options) {
        when(mTab.getUrl()).thenReturn(JUnitTestGURLs.SEARCH_URL);
        when(mTab.isNativePage()).thenReturn(options.isNativePage());
        doReturn(options.showTranslate())
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowTranslateMenuItem(any(Tab.class));
        doReturn(options.showUpdate())
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowUpdateMenuItem();
        doReturn(options.showMoveToOtherWindow())
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowMoveToOtherWindow();
        doReturn(options.showReaderModePrefs())
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowReaderModePrefs(any(Tab.class));
        doReturn(options.showPaintPreview())
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowPaintPreview(anyBoolean(), any(Tab.class));
        when(mWebsitePreferenceBridgeJniMock.getContentSetting(any(), anyInt(), any(), any()))
                .thenReturn(
                        options.isAutoDarkEnabled()
                                ? ContentSetting.DEFAULT
                                : ContentSetting.BLOCK);
    }
}

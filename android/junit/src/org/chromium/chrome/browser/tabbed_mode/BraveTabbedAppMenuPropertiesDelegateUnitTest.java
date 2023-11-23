/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.ContextThemeWrapper;
import android.view.Menu;
import android.view.View;
import android.widget.PopupMenu;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.FeatureList;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.JniMocker;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.appmenu.BraveTabbedAppMenuPropertiesDelegate;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.PowerBookmarkUtils;
import org.chromium.chrome.browser.commerce.ShoppingServiceFactory;
import org.chromium.chrome.browser.enterprise.util.ManagedBrowserUtils;
import org.chromium.chrome.browser.enterprise.util.ManagedBrowserUtilsJni;
import org.chromium.chrome.browser.feed.webfeed.WebFeedSnackbarController;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.offlinepages.OfflinePageUtils;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileJni;
import org.chromium.chrome.browser.readaloud.ReadAloudController;
import org.chromium.chrome.browser.signin.services.IdentityServicesProvider;
import org.chromium.chrome.browser.signin.services.SigninManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelFilter;
import org.chromium.chrome.browser.tabmodel.TabModelFilterProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.components.browser_ui.accessibility.PageZoomCoordinator;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridge;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridgeJni;
import org.chromium.components.commerce.core.ShoppingService;
import org.chromium.components.power_bookmarks.PowerBookmarkMeta;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.webapps.AppBannerManager;
import org.chromium.components.webapps.AppBannerManagerJni;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.url.JUnitTestGURLs;

/** Unit tests for {@link BraveTabbedAppMenuPropertiesDelegate}. */
@RunWith(BaseRobolectricTestRunner.class)
@EnableFeatures({
    ChromeFeatureList.WEB_FEED,
    ChromeFeatureList.BOOKMARKS_REFRESH,
    BraveFeatureList.NATIVE_BRAVE_WALLET
})
@DisableFeatures({ChromeFeatureList.SHOPPING_LIST})
public class BraveTabbedAppMenuPropertiesDelegateUnitTest {

    @Rule public JniMocker jniMocker = new JniMocker();
    @Rule public TestRule mProcessor = new Features.JUnitProcessor();
    @Mock private ActivityTabProvider mActivityTabProvider;
    @Mock private Tab mTab;
    @Mock private WebContents mWebContents;
    @Mock private NavigationController mNavigationController;
    @Mock private MultiWindowModeStateDispatcher mMultiWindowModeStateDispatcher;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private TabModel mTabModel;
    @Mock private ToolbarManager mToolbarManager;
    @Mock private View mDecorView;
    @Mock private LayoutStateProvider mLayoutStateProvider;
    @Mock private ManagedBrowserUtils.Natives mManagedBrowserUtilsJniMock;
    @Mock private Profile mProfile;
    @Mock private AppMenuDelegate mAppMenuDelegate;
    @Mock Profile.Natives mProfileJniMock;
    @Mock Profile mProfileMock;
    @Mock private WebFeedSnackbarController.FeedLauncher mFeedLauncher;
    @Mock private ModalDialogManager mDialogManager;
    @Mock private SnackbarManager mSnackbarManager;
    @Mock private OfflinePageUtils.Internal mOfflinePageUtils;
    @Mock private SigninManager mSigninManager;
    @Mock private IdentityManager mIdentityManager;
    @Mock private IdentityServicesProvider mIdentityService;
    @Mock private TabModelFilterProvider mTabModelFilterProvider;
    @Mock private TabModelFilter mTabModelFilter;
    @Mock public WebsitePreferenceBridge.Natives mWebsitePreferenceBridgeJniMock;
    @Mock private IncognitoReauthController mIncognitoReauthControllerMock;
    @Mock private ShoppingService mShoppingService;
    @Mock private AppBannerManager.Natives mAppBannerManagerJniMock;
    @Mock private ReadAloudController mReadAloudController;

    private OneshotSupplierImpl<LayoutStateProvider> mLayoutStateProviderSupplier =
            new OneshotSupplierImpl<>();
    private OneshotSupplierImpl<IncognitoReauthController> mIncognitoReauthControllerSupplier =
            new OneshotSupplierImpl<>();
    private ObservableSupplierImpl<BookmarkModel> mBookmarkModelSupplier =
            new ObservableSupplierImpl<>();
    private ObservableSupplierImpl<ReadAloudController> mReadAloudControllerSupplier =
            new ObservableSupplierImpl<>();

    private BraveTabbedAppMenuPropertiesDelegate mTabbedAppMenuPropertiesDelegate;

    Context mContext;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mLayoutStateProviderSupplier.set(mLayoutStateProvider);
        mIncognitoReauthControllerSupplier.set(mIncognitoReauthControllerMock);
        mReadAloudControllerSupplier.set(mReadAloudController);
        when(mTab.getWebContents()).thenReturn(mWebContents);
        when(mWebContents.getNavigationController()).thenReturn(mNavigationController);
        when(mNavigationController.getUseDesktopUserAgent()).thenReturn(false);
        when(mTabModelSelector.getCurrentModel()).thenReturn(mTabModel);
        when(mTabModelSelector.getModel(false)).thenReturn((mTabModel));
        when(mTabModel.isIncognito()).thenReturn(false);
        when(mTabModelSelector.getTabModelFilterProvider()).thenReturn(mTabModelFilterProvider);
        when(mTabModelFilterProvider.getCurrentTabModelFilter()).thenReturn(mTabModelFilter);
        when(mTabModelFilter.getTabModel()).thenReturn(mTabModel);
        jniMocker.mock(ProfileJni.TEST_HOOKS, mProfileJniMock);
        when(mProfileJniMock.fromWebContents(any(WebContents.class))).thenReturn(mProfileMock);
        jniMocker.mock(ManagedBrowserUtilsJni.TEST_HOOKS, mManagedBrowserUtilsJniMock);
        Profile.setLastUsedProfileForTesting(mProfile);
        jniMocker.mock(WebsitePreferenceBridgeJni.TEST_HOOKS, mWebsitePreferenceBridgeJniMock);
        OfflinePageUtils.setInstanceForTesting(mOfflinePageUtils);
        when(mIdentityService.getSigninManager(any(Profile.class))).thenReturn(mSigninManager);
        when(mSigninManager.getIdentityManager()).thenReturn(mIdentityManager);
        IdentityServicesProvider.setInstanceForTests(mIdentityService);
        FeatureList.setTestCanUseDefaultsForTesting();
        PageZoomCoordinator.setShouldShowMenuItemForTesting(false);
        jniMocker.mock(AppBannerManagerJni.TEST_HOOKS, mAppBannerManagerJniMock);
        Mockito.when(mAppBannerManagerJniMock.getInstallableWebAppManifestId(any()))
                .thenReturn(null);

        mContext =
                new ContextThemeWrapper(
                        ContextUtils.getApplicationContext(), R.style.Theme_BrowserUI_DayNight);
        PowerBookmarkUtils.setPriceTrackingEligibleForTesting(false);
        PowerBookmarkUtils.setPowerBookmarkMetaForTesting(PowerBookmarkMeta.newBuilder().build());
        mTabbedAppMenuPropertiesDelegate =
                Mockito.spy(
                        new BraveTabbedAppMenuPropertiesDelegate(
                                mContext,
                                mActivityTabProvider,
                                mMultiWindowModeStateDispatcher,
                                mTabModelSelector,
                                mToolbarManager,
                                mDecorView,
                                mAppMenuDelegate,
                                mLayoutStateProviderSupplier,
                                null,
                                mBookmarkModelSupplier,
                                mFeedLauncher,
                                mDialogManager,
                                mSnackbarManager,
                                mIncognitoReauthControllerSupplier,
                                mReadAloudControllerSupplier));
        SharedPreferencesManager.getInstance()
                .removeKeysWithPrefix(ChromePreferenceKeys.MULTI_INSTANCE_URL);
        SharedPreferencesManager.getInstance()
                .removeKeysWithPrefix(ChromePreferenceKeys.MULTI_INSTANCE_TAB_COUNT);

        ShoppingServiceFactory.setShoppingServiceForTesting(mShoppingService);
    }

    @Test
    @Config(qualifiers = "sw320dp")
    public void brave_testPageMenuItem_leo_normalTab() {
        // In this branch BraveTabbedAppMenuPropertiesDelegate.prepareMenu
        // has a lot of code disabled to make the test work
        // This was done for static methods, because the way to mock static methods doesn't work
        // try (MockedStatic<BraveRewardsNativeWorker> mockedBraveRewardsNativeWorker =
        // Mockito.mockStatic(BraveRewardsNativeWorker.class),
        //      MockedStatic<BraveVpnUtils> mockedBraveVpnUtils =
        // Mockito.mockStatic(BraveVpnUtils.class)) {
        //     mockedBraveVpnUtils.when(() ->
        // BraveVpnUtils.isVpnFeatureSupported(mContext)).thenReturn(false);
        //
        // mockedBraveRewardsNativeWorker.when(BraveRewardsNativeWorker::getInstance).thenReturn(null);
        //     // ...
        // }
        // Causes the exception
        // org.mockito.exceptions.base.MockitoException:
        // The used MockMaker SubclassByteBuddyMockMaker does not support the creation of static
        // mocks
        // Mockito's inline mock maker supports static mocks based on the Instrumentation API.
        // You can simply enable this mock mode, by placing the 'mockito-inline' artifact where you
        // are currently using 'mockito-core'.
        // Note that Mockito's inline mock maker is not supported on Android.
        //
        // Two other things which cause troubles, but probably can be solved with mocks of
        // corresponsing methods:
        // 1. Java asm patching is applied, I can see
        //
        // `//chrome/android:chrome_java__bytecode_rewrite(//build/toolchain/android:android_clang_x86)`
        //    message in build log, but `new TabbedAppMenuPropertiesDelegate` created
        // `TabbedAppMenuPropertiesDelegate`
        //    instead of `BraveTabbedAppMenuPropertiesDelegate`
        // 2. JNI calls doesn't happen

        setUpMocksForPageMenu();
        Menu menu = createTestMenu();
        mTabbedAppMenuPropertiesDelegate.prepareMenu(menu, null);
        assertMenuContains(menu, R.id.brave_leo_id);
    }

    @Test
    @Config(qualifiers = "sw320dp")
    public void brave_testPageMenuItem_leo_incognitoTab() {
        setUpMocksForPageMenu();
        when(mTab.isIncognito()).thenReturn(true);
        Menu menu = createTestMenu();
        mTabbedAppMenuPropertiesDelegate.prepareMenu(menu, null);
        assertMenuDoesNotContain(menu, R.id.brave_leo_id);
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
        doReturn(false).when(mTabbedAppMenuPropertiesDelegate).isPartnerHomepageEnabled();
        doReturn(true).when(mTabbedAppMenuPropertiesDelegate).isTabletSizeScreen();
        doReturn(true).when(mTabbedAppMenuPropertiesDelegate).isAutoDarkWebContentsEnabled();

        setUpIncognitoMocks();
        when(mTab.getUrl()).thenReturn(JUnitTestGURLs.SEARCH_URL);
        when(mTab.isNativePage()).thenReturn(false);
        doReturn(false)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowPaintPreview(anyBoolean(), any(Tab.class), anyBoolean());
        doReturn(true)
                .when(mTabbedAppMenuPropertiesDelegate)
                .shouldShowTranslateMenuItem(any(Tab.class));
        doReturn(
                        new AppBannerManager.InstallStringPair(
                                R.string.menu_add_to_homescreen, R.string.add))
                .when(mTabbedAppMenuPropertiesDelegate)
                .getAddToHomeScreenTitle(mTab);
        when(mManagedBrowserUtilsJniMock.isBrowserManaged(any())).thenReturn(true);
    }

    private void setUpIncognitoMocks() {
        doReturn(true).when(mTabbedAppMenuPropertiesDelegate).isIncognitoEnabled();
        doReturn(false).when(mIncognitoReauthControllerMock).isIncognitoReauthPending();
    }

    private Menu createTestMenu() {
        PopupMenu tempMenu = new PopupMenu(ContextUtils.getApplicationContext(), mDecorView);
        tempMenu.inflate(R.menu.brave_main_menu);
        return tempMenu.getMenu();
    }

    private boolean isMenuVisible(Menu menu, int itemId) {
        boolean found = false;
        for (int i = 0; i < menu.size(); i++) {
            if (menu.getItem(i).isVisible() && menu.getItem(i).getItemId() == itemId) {
                found = true;
                break;
            }
        }
        return found;
    }

    private void assertMenuContains(Menu menu, int itemId) {
        Assert.assertTrue(
                "Item should must be contained in the menu: " + itemId,
                isMenuVisible(menu, itemId));
    }

    private void assertMenuDoesNotContain(Menu menu, int itemId) {
        Assert.assertFalse(
                "Item should must not be contained in the menu: " + itemId,
                isMenuVisible(menu, itemId));
    }
}

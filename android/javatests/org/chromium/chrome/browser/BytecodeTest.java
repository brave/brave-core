/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.support.test.filters.SmallTest;
import android.view.View;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.init.StartupTabPreloader;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.AsyncTabParamsManager;
import org.chromium.chrome.browser.tabmodel.ChromeTabCreator;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

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
        Assert.assertTrue(classExists("org/chromium/base/CommandLineInitUtil"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/ui/appmenu/AppMenu"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/bottom/BottomControlsCoordinator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/toolbar/ToolbarManager"));
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
        Assert.assertTrue(classExists("org/chromium/chrome/browser/app/BraveActivity"));
        Assert.assertTrue(
                classExists("org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tabbed_mode/BraveTabbedRootUiCoordinator"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate"));
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/appmenu/BraveTabbedAppMenuPropertiesDelegate"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tabmodel/ChromeTabCreator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/tabmodel/BraveTabCreator"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BraveBookmarkUtils"));
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/sync/AndroidSyncSettings",
                "notifyObservers", false, null));
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
                "addBookmarkInternal", false, null));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkUtils",
                "createSnackbarControllerForEditButton", false, null));
    }

    @Test
    @SmallTest
    public void testMethodsForInvocationExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ChromeTabbedActivity",
                "hideOverview", true, void.class));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/app/BraveActivity",
                "openNewOrSelectExistingTab", true, Tab.class, String.class));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/app/BraveActivity",
                "selectExistingTab", true, Tab.class, String.class));
    }

    @Test
    @SmallTest
    public void testConstructorsExistAndMatch() throws Exception {
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator",
                "org/chromium/chrome/browser/tabbed_mode/BraveTabbedRootUiCoordinator",
                ChromeActivity.class, Callback.class, OneshotSupplier.class,
                ObservableSupplier.class, ActivityTabProvider.class, ObservableSupplierImpl.class,
                ObservableSupplier.class, ObservableSupplier.class, OneshotSupplier.class,
                Supplier.class, ObservableSupplier.class, OneshotSupplier.class,
                OneshotSupplier.class, Supplier.class));
        Assert.assertTrue(constructorsMatch(
                "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate",
                "org/chromium/chrome/browser/appmenu/BraveTabbedAppMenuPropertiesDelegate",
                Context.class, ActivityTabProvider.class, MultiWindowModeStateDispatcher.class,
                TabModelSelector.class, ToolbarManager.class, View.class, AppMenuDelegate.class,
                OneshotSupplier.class, ObservableSupplier.class, ModalDialogManager.class));
        Assert.assertTrue(constructorsMatch("org/chromium/chrome/browser/tabmodel/ChromeTabCreator",
                "org/chromium/chrome/browser/tabmodel/BraveTabCreator", ChromeActivity.class,
                WindowAndroid.class, StartupTabPreloader.class, Supplier.class, boolean.class,
                ChromeTabCreator.OverviewNTPCreator.class, AsyncTabParamsManager.class));
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
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "mShareDelegateSupplier"));
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
            Constructor ctor1 = c1.getConstructor(parameterTypes);
            Constructor ctor2 = c2.getConstructor(parameterTypes);
            if (ctor1 != null && ctor2 != null) {
                return true;
            }
        } catch (NoSuchMethodException e) {
            return false;
        }
        return false;
    }
}

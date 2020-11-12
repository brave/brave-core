/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

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
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/sync/AndroidSyncSettings", "notifyObservers"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/sync/AndroidSyncSettings", "updateCachedSettings"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/sync/AndroidSyncSettings", "setChromeSyncEnabled"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "extensiveBookmarkChangesBeginning"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "extensiveBookmarkChangesEnded"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/bookmarks/BookmarkBridge", "createBookmarkItem"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/LaunchIntentDispatcher", "isCustomTabIntent"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/homepage/HomepageManager",
                "shouldCloseAppWithZeroTabs"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/ntp/NewTabPageLayout", "insertSiteSectionView"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ntp/NewTabPageLayout",
                "getMaxRowsForMostVisitedTiles"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "getPermissionsLinkMessage"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "getSearchEngineSourceType"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter",
                "sortAndFilterUnnecessaryTemplateUrl"));
        Assert.assertTrue(methodExists("org/chromium/base/CommandLineInitUtil", "initCommandLine"));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/ui/appmenu/AppMenu", "getPopupPosition"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "onOrientationChange"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "updateButtonStatus"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/toolbar/ToolbarManager",
                "updateBookmarkButtonStatus"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/toolbar/ToolbarManager", "updateReloadState"));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "updateNewTabButtonVisibility"));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone",
                        "shouldShowIncognitoToggle"));
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/download/MimeUtils", "canAutoOpenMimeType"));
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
                "mBottomControlsCoordinator"));
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

    private boolean methodExists(String className, String methodName) {
        Class c = getClassForPath(className);
        if (c == null) {
            return false;
        }
        for (Method m : c.getDeclaredMethods()) {
            if (m.getName().equals(methodName)) {
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
}

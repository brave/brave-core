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
        Assert.assertTrue(classExists(
                "org/chromium/chrome/browser/toolbar/bottom/BottomToolbarConfiguration"));
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "notifyObservers"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "updateCachedSettings"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "setChromeSyncEnabled"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "extensiveBookmarkChangesBeginning"));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge", "extensiveBookmarkChangesEnded"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge", "createBookmarkItem"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/LaunchIntentDispatcher", "isCustomTabIntent"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/homepage/HomepageManager", "shouldCloseAppWithZeroTabs"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ntp/NewTabPageLayout", "insertSiteSectionView"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/ntp/NewTabPageLayout", "getMaxRowsForMostVisitedTiles"));
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
        Assert.assertTrue(methodExists(
                "org/chromium/chrome/browser/toolbar/bottom/BottomToolbarConfiguration",
                "isBottomToolbarEnabled"));
    }

    @Test
    @SmallTest
    public void testFieldsExist() throws Exception {
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "mIsSyncable"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "mChromeSyncEnabled"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/sync/AndroidSyncSettings", "mMasterSyncEnabled"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/ntp/NewTabPageLayout", "mSiteSectionView"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mActivity"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mScrollViewForPolicy"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mNtpHeader"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/feed/FeedSurfaceCoordinator", "mRootView"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mNewTabPageLayout"));
        Assert.assertTrue(fieldExists("org/chromium/chrome/browser/ntp/NewTabPage", "mFeedSurfaceProvider"));
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

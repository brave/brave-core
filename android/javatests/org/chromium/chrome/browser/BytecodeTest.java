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
        Assert.assertTrue(classExists("org/chromium/components/sync/AndroidSyncSettings"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge"));
        Assert.assertTrue(classExists("org/chromium/components/external_intents/ExternalNavigationHandler"));
        Assert.assertTrue(classExists("org/chromium/chrome/browser/LaunchIntentDispatcher"));
    }

    @Test
    @SmallTest
    public void testMethodsExist() throws Exception {
        Assert.assertTrue(methodExists("org/chromium/components/sync/AndroidSyncSettings", "notifyObservers"));
        Assert.assertTrue(methodExists("org/chromium/components/sync/AndroidSyncSettings", "updateCachedSettings"));
        Assert.assertTrue(methodExists("org/chromium/components/sync/AndroidSyncSettings", "setChromeSyncEnabled"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge",
                "extensiveBookmarkChangesBeginning"));
        Assert.assertTrue(
                methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge", "extensiveBookmarkChangesEnded"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/bookmarks/BookmarkBridge", "createBookmarkItem"));
        Assert.assertTrue(methodExists("org/chromium/components/external_intents/ExternalNavigationHandler",
                "clobberCurrentTabWithFallbackUrl"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/LaunchIntentDispatcher", "isCustomTabIntent"));
        Assert.assertTrue(methodExists("org/chromium/chrome/browser/homepage/HomepageManager", "shouldCloseAppWithZeroTabs"));
    }

    @Test
    @SmallTest
    public void testFieldsExist() throws Exception {
        Assert.assertTrue(fieldExists("org/chromium/components/sync/AndroidSyncSettings", "mIsSyncable"));
        Assert.assertTrue(fieldExists("org/chromium/components/sync/AndroidSyncSettings", "mChromeSyncEnabled"));
        Assert.assertTrue(fieldExists("org/chromium/components/sync/AndroidSyncSettings", "mMasterSyncEnabled"));
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

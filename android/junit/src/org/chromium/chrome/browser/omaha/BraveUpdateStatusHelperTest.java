/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omaha;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.SharedPreferences;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.ApkInfo;
import org.chromium.base.ContextUtils;
import org.chromium.base.IApkInfo;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.RobolectricUtil;

import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Supplier;

/** Tests that Play update info fed by {@link BraveUpdateStatusHelper} drives upstream's checks. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveUpdateStatusHelperTest {
    private static final int CURRENT_VERSION_CODE = 425010145;

    private IApkInfo mOriginalApkInfo;

    @Before
    public void setUp() {
        mOriginalApkInfo = ApkInfo.getAidlInfo();
        IApkInfo info = copy(mOriginalApkInfo);
        info.packageVersionCode = String.valueOf(CURRENT_VERSION_CODE);
        // Unrelated to the code on purpose: names must not be compared.
        info.packageVersionName = "999.1.101.45";
        ApkInfo.initFromParentProcess(info);
        prefs().edit().clear().commit();
    }

    @After
    public void tearDown() {
        prefs().edit().clear().commit();
        ApkInfo.initFromParentProcess(mOriginalApkInfo);
    }

    @Test
    public void testSetLatestVersionCode_usesOwnPackageMarketUrl() {
        BraveUpdateStatusHelper.setLatestVersionCode(CURRENT_VERSION_CODE + 1);
        RobolectricUtil.runAllBackgroundAndUi();

        String expectedUrl =
                "market://details?id=" + ContextUtils.getApplicationContext().getPackageName();
        assertEquals(expectedUrl, prefs().getString(OmahaPrefUtils.PREF_MARKET_URL, null));
        assertEquals(expectedUrl, runOffUiThread(MarketURLGetter::getMarketUrl));
    }

    @Test
    public void testSetLatestVersionCode_storesVersionCode() {
        BraveUpdateStatusHelper.setLatestVersionCode(CURRENT_VERSION_CODE + 1);
        RobolectricUtil.runAllBackgroundAndUi();

        assertEquals(
                "0.0.0." + (CURRENT_VERSION_CODE + 1),
                prefs().getString(OmahaPrefUtils.PREF_LATEST_VERSION, null));
    }

    @Test
    public void testCurrentlyUsedVersion_isVersionCode() {
        assertEquals(
                "0.0.0." + CURRENT_VERSION_CODE,
                new VersionNumberGetter().getCurrentlyUsedVersion());
    }

    @Test
    public void testNewerVersionCode_isNewer() {
        BraveUpdateStatusHelper.setLatestVersionCode(CURRENT_VERSION_CODE + 1);
        RobolectricUtil.runAllBackgroundAndUi();

        assertTrue(isNewerVersionAvailable());
    }

    @Test
    public void testSameVersionCode_isNotNewer() {
        BraveUpdateStatusHelper.setLatestVersionCode(CURRENT_VERSION_CODE);
        RobolectricUtil.runAllBackgroundAndUi();

        assertFalse(isNewerVersionAvailable());
    }

    @Test
    public void testOlderVersionCode_isNotNewer() {
        BraveUpdateStatusHelper.setLatestVersionCode(CURRENT_VERSION_CODE - 1);
        RobolectricUtil.runAllBackgroundAndUi();

        assertFalse(isNewerVersionAvailable());
    }

    @Test
    public void testClearLatestVersion_isNotNewer() {
        BraveUpdateStatusHelper.setLatestVersionCode(CURRENT_VERSION_CODE + 1);
        RobolectricUtil.runAllBackgroundAndUi();
        BraveUpdateStatusHelper.clearLatestVersion();
        RobolectricUtil.runAllBackgroundAndUi();

        assertNull(prefs().getString(OmahaPrefUtils.PREF_LATEST_VERSION, null));
        assertFalse(isNewerVersionAvailable());
    }

    private static boolean isNewerVersionAvailable() {
        return runOffUiThread(VersionNumberGetter::isNewerVersionAvailable);
    }

    // Omaha getters assert they are called off the UI thread.
    private static <T> T runOffUiThread(Supplier<T> supplier) {
        AtomicReference<T> result = new AtomicReference<>();
        PostTask.postTask(TaskTraits.USER_BLOCKING_MAY_BLOCK, () -> result.set(supplier.get()));
        RobolectricUtil.runAllBackgroundAndUi();
        return result.get();
    }

    private static SharedPreferences prefs() {
        return OmahaPrefUtils.getSharedPreferences();
    }

    private static IApkInfo copy(IApkInfo other) {
        IApkInfo info = new IApkInfo();
        info.hostPackageLabel = other.hostPackageLabel;
        info.hostPackageName = other.hostPackageName;
        info.hostVersionCode = other.hostVersionCode;
        info.installerPackageName = other.installerPackageName;
        info.isDebugApp = other.isDebugApp;
        info.isSystemApp = other.isSystemApp;
        info.packageName = other.packageName;
        info.packageVersionCode = other.packageVersionCode;
        info.packageVersionName = other.packageVersionName;
        info.targetSdkVersion = other.targetSdkVersion;
        return info;
    }
}

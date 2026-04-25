/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.set_default_browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.embedder_support.util.UrlConstants;

/**
 * Tests for {@link BraveSetDefaultBrowserUtils}. Verifies that the default browser status checking
 * methods correctly detect the default browser state.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@Batch(Batch.PER_CLASS)
public class BraveSetDefaultBrowserUtilsTest {
    private Context mContext;

    @Before
    public void setUp() {
        mContext = ContextUtils.getApplicationContext();
        assertNotNull("Context should not be null", mContext);
    }

    @Test
    @SmallTest
    public void testIsBraveSetAsDefaultBrowser_MatchesSystemResolvedActivity() {
        // This test verifies that isBraveSetAsDefaultBrowser correctly checks the resolved
        // activity against Brave package names.
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        ResolveInfo resolveInfo =
                mContext.getPackageManager()
                        .resolveActivity(browserIntent, PackageManager.MATCH_DEFAULT_ONLY);

        // The resolved activity should exist on any device with a browser.
        assertNotNull("System should have a default browser handler", resolveInfo);
        assertNotNull("ResolveInfo should have activityInfo", resolveInfo.activityInfo);
        assertNotNull("ActivityInfo should have packageName", resolveInfo.activityInfo.packageName);

        String defaultBrowserPackage = resolveInfo.activityInfo.packageName;

        // Check if the default browser is a Brave variant.
        boolean expectedResult =
                defaultBrowserPackage.equals(BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME)
                        || defaultBrowserPackage.equals(BraveConstants.BRAVE_BETA_PACKAGE_NAME)
                        || defaultBrowserPackage.equals(BraveConstants.BRAVE_NIGHTLY_PACKAGE_NAME)
                        || defaultBrowserPackage.equals(BraveConstants.BRAVE_DEBUG_PACKAGE_NAME);

        boolean actualResult = BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(mContext);

        assertEquals(
                "isBraveSetAsDefaultBrowser should return true only if default browser is a "
                        + "Brave variant. Current default: "
                        + defaultBrowserPackage,
                expectedResult,
                actualResult);
    }

    @Test
    @SmallTest
    public void testIsAppSetAsDefaultBrowser_MatchesCurrentPackage() {
        // This test verifies that isAppSetAsDefaultBrowser returns true only when
        // the current app's package is the default browser.
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        ResolveInfo resolveInfo =
                mContext.getPackageManager()
                        .resolveActivity(browserIntent, PackageManager.MATCH_DEFAULT_ONLY);

        assertNotNull("System should have a default browser handler", resolveInfo);

        String defaultBrowserPackage = resolveInfo.activityInfo.packageName;
        String currentPackage = mContext.getPackageName();

        boolean expectedResult = defaultBrowserPackage.equals(currentPackage);
        boolean actualResult = BraveSetDefaultBrowserUtils.isAppSetAsDefaultBrowser(mContext);

        assertEquals(
                "isAppSetAsDefaultBrowser should return true only if current package ("
                        + currentPackage
                        + ") matches default browser ("
                        + defaultBrowserPackage
                        + ")",
                expectedResult,
                actualResult);
    }

    @Test
    @SmallTest
    public void testBravePackageNames_AreCorrectlyDefined() {
        // This test verifies the Brave package name constants are properly defined
        // and match the expected format.
        assertEquals(
                "Production package name should be com.brave.browser",
                "com.brave.browser",
                BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME);
        assertEquals(
                "Beta package name should be com.brave.browser_beta",
                "com.brave.browser_beta",
                BraveConstants.BRAVE_BETA_PACKAGE_NAME);
        assertEquals(
                "Nightly package name should be com.brave.browser_nightly",
                "com.brave.browser_nightly",
                BraveConstants.BRAVE_NIGHTLY_PACKAGE_NAME);
        assertEquals(
                "Debug package name should be com.brave.browser_default",
                "com.brave.browser_default",
                BraveConstants.BRAVE_DEBUG_PACKAGE_NAME);
    }
}

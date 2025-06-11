/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.site_settings.SiteSettings;

@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveSiteSettingsTest {
    private static final String DIVIDER_KEY = "divider";
    private static final String PERMISSION_AUTOREVOCATION_KEY = "permission_autorevocation";
    private static final String SOLANA_CONNECTED_SITES_KEY = "solana_connected_sites";

    /**
     * Tests an order of PERMISSION_AUTOREVOCATION_KEY. It has to be at the bottom, right after
     * SOLANA_CONNECTED_SITES_KEY
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testSiteSettingsMenuPermissionAutorevocationOrder() {
        final SettingsActivity settingsActivity = SiteSettingsTestUtils.startSiteSettingsMenu("");
        SiteSettings siteSettings = (SiteSettings) settingsActivity.getMainFragment();
        Assert.assertNotNull(siteSettings);
        Preference prefDivider = siteSettings.findPreference(DIVIDER_KEY);
        Assert.assertNotNull(prefDivider);
        Preference prefPermissionAutorevocation =
                siteSettings.findPreference(PERMISSION_AUTOREVOCATION_KEY);
        Assert.assertNotNull(prefPermissionAutorevocation);
        Preference prefSolanaConnectedSites =
                siteSettings.findPreference(SOLANA_CONNECTED_SITES_KEY);
        Assert.assertNotNull(prefSolanaConnectedSites);
        Assert.assertEquals(prefSolanaConnectedSites.getOrder(), prefDivider.getOrder() - 1);
        Assert.assertEquals(
                prefSolanaConnectedSites.getOrder(), prefPermissionAutorevocation.getOrder() - 2);
        settingsActivity.finish();
    }
}

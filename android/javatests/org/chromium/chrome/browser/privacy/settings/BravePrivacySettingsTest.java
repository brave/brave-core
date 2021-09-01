/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;

import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxBridge;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxSettingsFragment;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

// Checks if changes have been made to the Chromium privacy settings
@RunWith(ChromeJUnit4ClassRunner.class)
public class BravePrivacySettingsTest {
    // Chromium Prefs that are being checked
    private static final String PREF_CAN_MAKE_PAYMENT = "can_make_payment";
    private static final String PREF_NETWORK_PREDICTIONS = "preload_pages";
    private static final String PREF_SECURE_DNS = "secure_dns";
    private static final String PREF_USAGE_STATS = "usage_stats_reporting";
    private static final String PREF_DO_NOT_TRACK = "do_not_track";
    private static final String PREF_SAFE_BROWSING = "safe_browsing";
    private static final String PREF_SYNC_AND_SERVICES_LINK = "sync_and_services_link";
    private static final String PREF_CLEAR_BROWSING_DATA = "clear_browsing_data";
    private static final String PREF_PRIVACY_SANDBOX = "privacy_sandbox";

    // Ignore usage_stats_reporting and privacy_sandbox
    // Items counted:
    // - clear_browsing_data
    // - safe_browsing
    // - https_first_mode
    // - can_make_payment
    // - preload_pages
    // - secure_dns
    // - do_not_track
    // - sync_and_services_link
    private static int PRIVACY_SETTINGS_NUMBER_OF_ITEMS = 8;
    private static int BRAVE_PRIVACY_SETTINGS_NUMBER_OF_ITEMS = 20;

    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    @Rule
    public SettingsActivityTestRule<BravePrivacySettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BravePrivacySettings.class);
    private BravePrivacySettings mFragment;

    @Before
    public void setUp() {
        mSettingsActivityTestRule.startSettingsActivity();
        mFragment = (BravePrivacySettings) mSettingsActivityTestRule.getFragment();
    }

    @Test
    @SmallTest
    public void testNumberOfItemsNotChanged() {
        assertEquals(PRIVACY_SETTINGS_NUMBER_OF_ITEMS + BRAVE_PRIVACY_SETTINGS_NUMBER_OF_ITEMS,
                mFragment.getPreferenceScreen().getPreferenceCount());
    }

    @Test
    @SmallTest
    public void testParentItems() {
        assertNotEquals(null, mFragment.findPreference(PREF_CAN_MAKE_PAYMENT));
        assertNotEquals(null, mFragment.findPreference(PREF_SECURE_DNS));
        assertNotEquals(null, mFragment.findPreference(PREF_DO_NOT_TRACK));
        assertNotEquals(null, mFragment.findPreference(PREF_SAFE_BROWSING));
        assertNotEquals(null, mFragment.findPreference(PREF_CLEAR_BROWSING_DATA));
        // These parent items should be removed
        assertEquals(null, mFragment.findPreference(PREF_NETWORK_PREDICTIONS));
        assertEquals(null, mFragment.findPreference(PREF_USAGE_STATS));
        assertEquals(null, mFragment.findPreference(PREF_SYNC_AND_SERVICES_LINK));
        assertEquals(null, mFragment.findPreference(PREF_PRIVACY_SANDBOX));
    }

    @Test
    @SmallTest
    public void testDisabledOptions() {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            assertFalse(PrivacySandboxBridge.isPrivacySandboxSettingsFunctional());
            assertFalse(PrivacyPreferencesManagerImpl.getInstance().getNetworkPredictionEnabled());
        });
    }
}

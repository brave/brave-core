/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.Looper;

import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.tracing.settings.DeveloperSettings;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Test for {@link MainSettings}. Main purpose is to have a quick confidence check on the xml. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Tests cannot run batched because they launch a Settings activity.")
public class BraveMainSettingsFragmentTest {
    private static final String PREF_BRAVE_ORIGIN = "brave_origin";

    @Rule
    public final SettingsActivityTestRule<MainSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(MainSettings.class);

    private MainSettings mMainSettings;

    @Before
    public void setup() {
        Looper.prepare();
        DeveloperSettings.setIsEnabledForTests(true);
    }

    private static final String[] sSortedPrefKeys = {
        "features_section",
        "brave_shields_and_privacy",
        "brave_news_v2",
        "brave_wallet",
        "brave_vpn",
        "brave_leo",
        "general_section",
        "brave_search_engines",
        "homepage",
        "brave_sync_layout",
        "brave_stats",
        "content_settings",
        "brave_downloads",
        "closing_all_tabs_closes_brave",
        "use_custom_tabs",
        "display_section",
        "tabs",
        "media",
        "appearance",
        "background_images",
        "accessibility",
        "brave_languages",
        "autofill_section",
        "passwords",
        "autofill_options",
        "autofill_payment_methods",
        "autofill_addresses",
        "autofill_private_window",
        "support_section",
        "rate_brave",
        "about_section",
        "developer",
        "about_chrome"
    };

    // This test is intended to check the issue where prefs order was wrong
    // when app language was switched to some other than default English,
    // like French. The test infrastructure does not support switching app
    // interface language. So the test may help to catch order violation
    // with English.
    @Test
    @SmallTest
    public void testMainSettingsPrefsOrder() {
        startSettings();

        @Nullable Preference prevPref = null;
        for (int i = 0; i < sSortedPrefKeys.length; ++i) {
            Preference pref = mMainSettings.findPreference(sSortedPrefKeys[i]);

            if (prevPref == null) { // Skip first pref.
                prevPref = pref;
                continue;
            }
            assertTrue(
                    prevPref.getTitle() + " should precede " + pref.getTitle(),
                    pref.getOrder() > prevPref.getOrder());
        }
    }

    // Test for BraveOrigin pref when feature is disabled. It should not be shown.
    @Test
    @SmallTest
    @DisableFeatures(BraveFeatureList.BRAVE_ORIGIN)
    public void testBraveOriginPrefNotShownWhenFeatureDisabled() {
        Assert.assertFalse(
                "BraveOrigin feature should be disabled",
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ORIGIN));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BraveMainPreferencesBase.findPreference() also returns removed preferences
        CriteriaHelper.pollUiThread(
                () -> {
                    return mMainSettings.getPreferenceScreen().findPreference(PREF_BRAVE_ORIGIN)
                            == null;
                },
                "Preference should be removed when feature is disabled",
                5000L,
                100L);

        Preference braveOriginPref =
                mMainSettings.getPreferenceScreen().findPreference(PREF_BRAVE_ORIGIN);
        assertNull(
                "PREF_BRAVE_ORIGIN should not be shown when feature is disabled", braveOriginPref);
    }

    // Test for BraveOrigin pref when feature is enabled. It should be shown.
    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ORIGIN)
    public void testBraveOriginPrefShownWhenFeatureEnabled() {
        Assert.assertTrue(
                "BraveOrigin feature should be enabled",
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ORIGIN));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BraveMainPreferencesBase.findPreference() also returns removed preferences
        CriteriaHelper.pollUiThread(
                () -> {
                    return mMainSettings.getPreferenceScreen().findPreference(PREF_BRAVE_ORIGIN)
                            != null;
                },
                "Preference should be present when feature is enabled",
                5000L,
                100L);

        Preference braveOriginPref =
                mMainSettings.getPreferenceScreen().findPreference(PREF_BRAVE_ORIGIN);
        assertNotNull("PREF_BRAVE_ORIGIN should be shown when feature is enabled", braveOriginPref);
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mMainSettings = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mMainSettings);
    }
}

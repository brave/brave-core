/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.origin.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;

import android.os.Looper;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.DoNotBatch;
import org.chromium.chrome.browser.settings.BraveOriginPreferences;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/** Test for {@link BraveOriginPreferences}. Main purpose is to check preference defaults. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Tests cannot run batched because they launch a Settings activity.")
public class BraveOriginPreferencesTest {
    private static final String PREF_REWARDS_SWITCH = "rewards_switch";
    private static final String PREF_CRASH_REPORTS_SWITCH = "crash_reports_switch";
    private static final String PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH =
            "privacy_preserving_analytics_switch";
    private static final String PREF_EMAIL_ALIASES_SWITCH = "email_aliases_switch";
    private static final String PREF_LEO_AI_SWITCH = "leo_ai_switch";
    private static final String PREF_NEWS_SWITCH = "news_switch";
    private static final String PREF_STATISTICS_REPORTING_SWITCH = "statistics_reporting_switch";
    private static final String PREF_VPN_SWITCH = "vpn_switch";
    private static final String PREF_WALLET_SWITCH = "wallet_switch";
    private static final String PREF_WEB_DISCOVERY_PROJECT_SWITCH = "web_discovery_project_switch";

    @Rule
    public final SettingsActivityTestRule<BraveOriginPreferences> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveOriginPreferences.class);

    private BraveOriginPreferences mBraveOriginPreferences;

    @Before
    public void setup() {
        Looper.prepare();
    }

    // Test that all switch preferences are off by default.
    @Test
    @SmallTest
    public void testAllSwitchPreferencesDefaultValues() {
        startSettings();

        // Test rewards switch
        ChromeSwitchPreference rewardsSwitch =
                (ChromeSwitchPreference)
                        mBraveOriginPreferences.findPreference(PREF_REWARDS_SWITCH);
        assertNotNull("PREF_REWARDS_SWITCH should exist", rewardsSwitch);
        assertFalse("PREF_REWARDS_SWITCH should be off by default", rewardsSwitch.isChecked());

        // Test crash reports switch
        ChromeSwitchPreference crashReportsSwitch =
                (ChromeSwitchPreference)
                        mBraveOriginPreferences.findPreference(PREF_CRASH_REPORTS_SWITCH);
        assertNotNull("PREF_CRASH_REPORTS_SWITCH should exist", crashReportsSwitch);
        assertFalse(
                "PREF_CRASH_REPORTS_SWITCH should be off by default",
                crashReportsSwitch.isChecked());

        // Test privacy preserving analytics switch
        ChromeSwitchPreference privacyPreservingAnalyticsSwitch =
                (ChromeSwitchPreference)
                        mBraveOriginPreferences.findPreference(
                                PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH);
        assertNotNull(
                "PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH should exist",
                privacyPreservingAnalyticsSwitch);
        assertFalse(
                "PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH should be off by default",
                privacyPreservingAnalyticsSwitch.isChecked());

        // Test email aliases switch
        ChromeSwitchPreference emailAliasesSwitch =
                (ChromeSwitchPreference)
                        mBraveOriginPreferences.findPreference(PREF_EMAIL_ALIASES_SWITCH);
        assertNotNull("PREF_EMAIL_ALIASES_SWITCH should exist", emailAliasesSwitch);
        assertFalse(
                "PREF_EMAIL_ALIASES_SWITCH should be off by default",
                emailAliasesSwitch.isChecked());

        // Test Leo AI switch
        ChromeSwitchPreference leoAiSwitch =
                (ChromeSwitchPreference) mBraveOriginPreferences.findPreference(PREF_LEO_AI_SWITCH);
        assertNotNull("PREF_LEO_AI_SWITCH should exist", leoAiSwitch);
        assertFalse("PREF_LEO_AI_SWITCH should be off by default", leoAiSwitch.isChecked());

        // Test news switch
        ChromeSwitchPreference newsSwitch =
                (ChromeSwitchPreference) mBraveOriginPreferences.findPreference(PREF_NEWS_SWITCH);
        assertNotNull("PREF_NEWS_SWITCH should exist", newsSwitch);
        assertFalse("PREF_NEWS_SWITCH should be off by default", newsSwitch.isChecked());

        // Test statistics reporting switch
        ChromeSwitchPreference statisticsReportingSwitch =
                (ChromeSwitchPreference)
                        mBraveOriginPreferences.findPreference(PREF_STATISTICS_REPORTING_SWITCH);
        assertNotNull("PREF_STATISTICS_REPORTING_SWITCH should exist", statisticsReportingSwitch);
        assertFalse(
                "PREF_STATISTICS_REPORTING_SWITCH should be off by default",
                statisticsReportingSwitch.isChecked());

        // Test VPN switch
        ChromeSwitchPreference vpnSwitch =
                (ChromeSwitchPreference) mBraveOriginPreferences.findPreference(PREF_VPN_SWITCH);
        assertNotNull("PREF_VPN_SWITCH should exist", vpnSwitch);
        assertFalse("PREF_VPN_SWITCH should be off by default", vpnSwitch.isChecked());

        // Test wallet switch
        ChromeSwitchPreference walletSwitch =
                (ChromeSwitchPreference) mBraveOriginPreferences.findPreference(PREF_WALLET_SWITCH);
        assertNotNull("PREF_WALLET_SWITCH should exist", walletSwitch);
        assertFalse("PREF_WALLET_SWITCH should be off by default", walletSwitch.isChecked());

        // Test web discovery project switch
        ChromeSwitchPreference webDiscoveryProjectSwitch =
                (ChromeSwitchPreference)
                        mBraveOriginPreferences.findPreference(PREF_WEB_DISCOVERY_PROJECT_SWITCH);
        assertNotNull("PREF_WEB_DISCOVERY_PROJECT_SWITCH should exist", webDiscoveryProjectSwitch);
        assertFalse(
                "PREF_WEB_DISCOVERY_PROJECT_SWITCH should be off by default",
                webDiscoveryProjectSwitch.isChecked());
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mBraveOriginPreferences = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mBraveOriginPreferences);
    }
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.os.Looper;

import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.policy.PolicyServiceFactory;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tasks.tab_management.TabsSettings;
import org.chromium.chrome.browser.tracing.settings.DeveloperSettings;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.policy.PolicyService;

import java.util.stream.IntStream;

/** Test for {@link MainSettings}. Main purpose is to have a quick confidence check on the xml. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Tests cannot run batched because they launch a Settings activity.")
public class BraveMainSettingsFragmentTest {
    private static final String PREF_BRAVE_ORIGIN = "brave_origin";

    @Rule
    public final SettingsActivityTestRule<MainSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(MainSettings.class);

    private MainSettings mMainSettings;

    // Policy-controlled feature rows gated by updateFeaturePolicyPreferences(). Keys are
    // referenced from BraveMainPreferencesBase so they stay in sync if they change there.
    // These rows are always present, so they are asserted strictly.
    private static final String[] sAlwaysPresentPolicyControlledPrefKeys = {
        BraveMainPreferencesBase.PREF_BRAVE_NEWS_V2,
        BraveMainPreferencesBase.PREF_BRAVE_WALLET,
        BraveMainPreferencesBase.PREF_BRAVE_LEO
    };

    // VPN rows are also policy-controlled, but only present when VPN is supported (build/device
    // dependent), so they are asserted conditionally: when present they must follow the gate.
    private static final String[] sVpnPolicyControlledPrefKeys = {
        BraveMainPreferencesBase.PREF_BRAVE_VPN, BraveMainPreferencesBase.PREF_BRAVE_VPN_CALLOUT
    };

    @Before
    public void setup() {
        Looper.prepare();
        DeveloperSettings.setIsEnabledForTests(true);
    }

    @After
    public void tearDown() {
        // Undo the per-test PolicyService injection and Origin-active cache so they don't leak
        // into other (unbatched) tests in this process. removeKey() restores the natural unset
        // default rather than forcing a value.
        PolicyServiceFactory.setPolicyServiceForTest(null);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ORIGIN_CREDENTIAL_SUMMARY_CACHED);
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
        "brave_browser_lock",
        "closing_all_tabs_closes_brave",
        "display_section",
        "tabs",
        "media",
        "brave_appearance",
        "background_images",
        "accessibility",
        "brave_languages",
        "autofill_section",
        "passwords",
        "autofill_options",
        "autofill_payment_methods",
        "autofill_addresses",
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
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
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

    @Test
    @SmallTest
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testLegacyTabRowsShownWhenTabGroupSettingsFeatureDisabled() {
        startSettings();

        Preference tabsPreference = mMainSettings.findPreference(MainSettings.PREF_TABS);
        Preference closingAllTabsClosesBravePreference =
                mMainSettings
                        .getPreferenceScreen()
                        .findPreference(
                                BraveMainPreferencesBase.PREF_CLOSING_ALL_TABS_CLOSES_BRAVE);

        assertNotNull(tabsPreference);
        assertNotNull(closingAllTabsClosesBravePreference);
        assertTrue(closingAllTabsClosesBravePreference.isVisible());
        assertEquals(TabsSettings.class.getName(), tabsPreference.getFragment());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testLegacyTabRowsHiddenWhenTabGroupSettingsFeatureEnabled() {
        startSettings();

        Preference tabsPreference = mMainSettings.findPreference(MainSettings.PREF_TABS);
        Preference closingAllTabsClosesBravePreference =
                mMainSettings
                        .getPreferenceScreen()
                        .findPreference(
                                BraveMainPreferencesBase.PREF_CLOSING_ALL_TABS_CLOSES_BRAVE);

        assertNotNull(tabsPreference);
        assertNull(closingAllTabsClosesBravePreference);
        assertEquals(BraveTabsAndTabGroupsSettings.class.getName(), tabsPreference.getFragment());
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

    @Test
    @SmallTest
    public void testPreferenceCount() {
        startSettings();

        final int preferenceCount = mMainSettings.getPreferenceScreen().getPreferenceCount();

        // VPN prefs (brave_vpn, pref_vpn_callout) are only present when VPN is supported,
        // which depends on BraveRewards being enabled (disabled on x86 official/Release builds).
        // Exclude them so the assertion is stable across build types.
        long nonVpnCount =
                IntStream.range(0, preferenceCount)
                        .mapToObj(i -> mMainSettings.getPreferenceScreen().getPreference(i))
                        .filter(
                                p ->
                                        !p.getKey().equals("brave_vpn")
                                                && !p.getKey().equals("pref_vpn_callout"))
                        .count();

        // "closing_all_tabs_closes_brave" moves into the Brave "Tabs and tab groups" settings
        // screen when kBraveAndroidTabGroupsSettings is enabled, so it is removed from the main
        // settings screen in that case.
        int expectedCount =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
                        ? 33
                        : 34;

        assertEquals(
                "Number of preferences has changed, please check and update preferenceCount"
                    + " expectation here or modify BraveMainPreferencesBase.updateBravePreferences"
                    + " to remove it and BraveMainPreferencesBase.SEARCH_"
                        // Split that static field no to trigger presubmit CheckSettingsChanges
                        // check
                        + "INDEX_DATA_PROVIDER.updateDynamicPreferences"
                        + " to exclude new from indexing.",
                expectedCount,
                nonVpnCount);
    }

    // For a Brave Origin subscriber, the policy-controlled feature rows must stay hidden until the
    // profile policy service finishes applying Brave Origin policies. This guards against the race
    // (https://github.com/brave/brave-browser/issues/56156) where, after the app-language-change
    // restart recreates Settings, disabled features briefly appeared before policies were merged.
    @Test
    @SmallTest
    public void testPolicyControlledPrefsHiddenWhilePolicyServiceUninitialized() {
        markOriginSubscriberCached();
        PolicyService policyService = mock(PolicyService.class);
        when(policyService.isInitializationComplete()).thenReturn(false);
        PolicyServiceFactory.setPolicyServiceForTest(policyService);

        startSettings();

        CriteriaHelper.pollUiThread(
                this::arePolicyControlledPrefsHidden,
                "Policy-controlled prefs should be hidden while the policy service is"
                        + " uninitialized",
                5000L,
                100L);
    }

    // Counterpart to the test above: once the profile policy service reports initialization
    // complete, the policy-controlled rows that are not disabled by policy are shown.
    @Test
    @SmallTest
    public void testPolicyControlledPrefsShownWhenPolicyServiceInitialized() {
        markOriginSubscriberCached();
        PolicyService policyService = mock(PolicyService.class);
        when(policyService.isInitializationComplete()).thenReturn(true);
        PolicyServiceFactory.setPolicyServiceForTest(policyService);

        startSettings();

        CriteriaHelper.pollUiThread(
                this::arePolicyControlledPrefsShown,
                "Policy-controlled prefs should be shown once the policy service is initialized",
                5000L,
                100L);
    }

    private void markOriginSubscriberCached() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ORIGIN_CREDENTIAL_SUMMARY_CACHED, true);
    }

    private boolean arePolicyControlledPrefsHidden() {
        for (String key : sAlwaysPresentPolicyControlledPrefKeys) {
            if (!isPrefHidden(key)) {
                return false;
            }
        }
        // VPN rows are build-dependent: when present they must be hidden too.
        for (String key : sVpnPolicyControlledPrefKeys) {
            if (isPrefVisible(key)) {
                return false;
            }
        }
        return true;
    }

    private boolean arePolicyControlledPrefsShown() {
        for (String key : sAlwaysPresentPolicyControlledPrefKeys) {
            if (!isPrefVisible(key)) {
                return false;
            }
        }
        // VPN rows are build-dependent: when present they must be shown too.
        for (String key : sVpnPolicyControlledPrefKeys) {
            if (isPrefHidden(key)) {
                return false;
            }
        }
        return true;
    }

    private boolean isPrefVisible(String key) {
        Preference pref = mMainSettings.getPreferenceScreen().findPreference(key);
        return pref != null && pref.isVisible();
    }

    private boolean isPrefHidden(String key) {
        Preference pref = mMainSettings.getPreferenceScreen().findPreference(key);
        return pref != null && !pref.isVisible();
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mMainSettings = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mMainSettings);
    }
}

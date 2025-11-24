/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.os.Looper;

import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
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
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Test for {@link BackgroundImagesPreferences}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Tests cannot run batched because they launch a Settings activity.")
public class BackgroundImagesPreferencesTest {
    private static final String PREF_OPENING_SCREEN = "opening_screen_option";
    private static final String PREF_OPENING_SCREEN_CATEGORY = "opening_screen";

    @Rule
    public final SettingsActivityTestRule<BackgroundImagesPreferences> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BackgroundImagesPreferences.class);

    private BackgroundImagesPreferences mBackgroundImagesPreferences;

    @Before
    public void setup() {
        Looper.prepare();
    }

    // Test for Opening Screen preference when feature is disabled. It should not be shown.
    @Test
    @SmallTest
    @DisableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT)
    public void testOpeningScreenPrefNotShownWhenFeatureDisabled() {
        Assert.assertFalse(
                "BraveFreshNtpAfterIdleExperiment feature should be disabled",
                ChromeFeatureList.isEnabled(
                        BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BravePreferenceFragment.findPreference() may also return removed preferences
        CriteriaHelper.pollUiThread(
                () -> {
                    return mBackgroundImagesPreferences
                                    .getPreferenceScreen()
                                    .findPreference(PREF_OPENING_SCREEN_CATEGORY)
                            == null;
                },
                "Preference category should be removed when feature is disabled",
                5000L,
                100L);

        PreferenceCategory openingScreenCategory =
                (PreferenceCategory)
                        mBackgroundImagesPreferences
                                .getPreferenceScreen()
                                .findPreference(PREF_OPENING_SCREEN_CATEGORY);
        assertNull(
                "PREF_OPENING_SCREEN_CATEGORY should not be shown when feature is disabled",
                openingScreenCategory);

        // Also verify the preference itself is not accessible
        Preference openingScreenPref =
                mBackgroundImagesPreferences
                        .getPreferenceScreen()
                        .findPreference(PREF_OPENING_SCREEN);
        assertNull(
                "PREF_OPENING_SCREEN should not be shown when feature is disabled",
                openingScreenPref);
    }

    // Test for Opening Screen preference when feature is enabled but variant is "A".
    // The preference should not be shown when variant is "A" (the default).
    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT)
    public void testOpeningScreenPrefNotShownWhenFeatureEnabledButVariantIsA() {
        Assert.assertTrue(
                "BraveFreshNtpAfterIdleExperiment feature should be enabled",
                ChromeFeatureList.isEnabled(
                        BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BravePreferenceFragment.findPreference() may also return removed preferences
        // The preference should be removed when variant is "A" (default) even if feature is enabled
        CriteriaHelper.pollUiThread(
                () -> {
                    return mBackgroundImagesPreferences
                                    .getPreferenceScreen()
                                    .findPreference(PREF_OPENING_SCREEN_CATEGORY)
                            == null;
                },
                "Preference category should be removed when variant is A (default)",
                5000L,
                100L);

        PreferenceCategory openingScreenCategory =
                (PreferenceCategory)
                        mBackgroundImagesPreferences
                                .getPreferenceScreen()
                                .findPreference(PREF_OPENING_SCREEN_CATEGORY);
        assertNull(
                "PREF_OPENING_SCREEN_CATEGORY should not be shown when variant is A (default)",
                openingScreenCategory);

        // Also verify the preference itself is not accessible
        Preference openingScreenPref =
                mBackgroundImagesPreferences
                        .getPreferenceScreen()
                        .findPreference(PREF_OPENING_SCREEN);
        assertNull(
                "PREF_OPENING_SCREEN should not be shown when variant is A (default)",
                openingScreenPref);
    }

    // Test for Opening Screen preference when feature is enabled and variant is "B".
    // The preference should be shown when variant is not "A".
    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT + ":variant/B")
    public void testOpeningScreenPrefShownWhenFeatureEnabledAndVariantIsB() {
        Assert.assertTrue(
                "BraveFreshNtpAfterIdleExperiment feature should be enabled",
                ChromeFeatureList.isEnabled(
                        BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BravePreferenceFragment.findPreference() may also return removed preferences
        // The preference should be present when variant is not "A" and feature is enabled
        CriteriaHelper.pollUiThread(
                () -> {
                    return mBackgroundImagesPreferences
                                    .getPreferenceScreen()
                                    .findPreference(PREF_OPENING_SCREEN_CATEGORY)
                            != null;
                },
                "Preference category should be present when feature is enabled and variant is B",
                5000L,
                100L);

        PreferenceCategory openingScreenCategory =
                (PreferenceCategory)
                        mBackgroundImagesPreferences
                                .getPreferenceScreen()
                                .findPreference(PREF_OPENING_SCREEN_CATEGORY);
        assertNotNull(
                "PREF_OPENING_SCREEN_CATEGORY should be shown when feature is enabled and variant"
                        + " is B",
                openingScreenCategory);

        // Also verify the preference itself is accessible
        Preference openingScreenPref =
                mBackgroundImagesPreferences
                        .getPreferenceScreen()
                        .findPreference(PREF_OPENING_SCREEN);
        assertNotNull(
                "PREF_OPENING_SCREEN should be shown when feature is enabled and variant is B",
                openingScreenPref);
    }

    // Test for Opening Screen preference when feature is enabled and variant is "C".
    // The preference should be shown when variant is not "A".
    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT + ":variant/C")
    public void testOpeningScreenPrefShownWhenFeatureEnabledAndVariantIsC() {
        Assert.assertTrue(
                "BraveFreshNtpAfterIdleExperiment feature should be enabled",
                ChromeFeatureList.isEnabled(
                        BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BravePreferenceFragment.findPreference() may also return removed preferences
        // The preference should be present when variant is not "A" and feature is enabled
        CriteriaHelper.pollUiThread(
                () -> {
                    return mBackgroundImagesPreferences
                                    .getPreferenceScreen()
                                    .findPreference(PREF_OPENING_SCREEN_CATEGORY)
                            != null;
                },
                "Preference category should be present when feature is enabled and variant is C",
                5000L,
                100L);

        PreferenceCategory openingScreenCategory =
                (PreferenceCategory)
                        mBackgroundImagesPreferences
                                .getPreferenceScreen()
                                .findPreference(PREF_OPENING_SCREEN_CATEGORY);
        assertNotNull(
                "PREF_OPENING_SCREEN_CATEGORY should be shown when feature is enabled and variant"
                        + " is C",
                openingScreenCategory);

        // Also verify the preference itself is accessible
        Preference openingScreenPref =
                mBackgroundImagesPreferences
                        .getPreferenceScreen()
                        .findPreference(PREF_OPENING_SCREEN);
        assertNotNull(
                "PREF_OPENING_SCREEN should be shown when feature is enabled and variant is C",
                openingScreenPref);
    }

    // Test for Opening Screen preference when feature is enabled and variant is "D".
    // The preference should be shown when variant is not "A".
    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT + ":variant/D")
    public void testOpeningScreenPrefShownWhenFeatureEnabledAndVariantIsD() {
        Assert.assertTrue(
                "BraveFreshNtpAfterIdleExperiment feature should be enabled",
                ChromeFeatureList.isEnabled(
                        BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT));
        startSettings();

        // Wait for async preference updates to complete
        // Note: We check getPreferenceScreen().findPreference() directly because
        // BravePreferenceFragment.findPreference() may also return removed preferences
        // The preference should be present when variant is not "A" and feature is enabled
        CriteriaHelper.pollUiThread(
                () -> {
                    return mBackgroundImagesPreferences
                                    .getPreferenceScreen()
                                    .findPreference(PREF_OPENING_SCREEN_CATEGORY)
                            != null;
                },
                "Preference category should be present when feature is enabled and variant is D",
                5000L,
                100L);

        PreferenceCategory openingScreenCategory =
                (PreferenceCategory)
                        mBackgroundImagesPreferences
                                .getPreferenceScreen()
                                .findPreference(PREF_OPENING_SCREEN_CATEGORY);
        assertNotNull(
                "PREF_OPENING_SCREEN_CATEGORY should be shown when feature is enabled and variant"
                        + " is D",
                openingScreenCategory);

        // Also verify the preference itself is accessible
        Preference openingScreenPref =
                mBackgroundImagesPreferences
                        .getPreferenceScreen()
                        .findPreference(PREF_OPENING_SCREEN);
        assertNotNull(
                "PREF_OPENING_SCREEN should be shown when feature is enabled and variant is D",
                openingScreenPref);
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mBackgroundImagesPreferences = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mBackgroundImagesPreferences);
    }
}

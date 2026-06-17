/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

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
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.appearance.settings.AppearanceSettingsFragment;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Test for {@link AppearancePreferences}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Tests cannot run batched because they launch a Settings activity.")
public class BraveAppearancePreferencesTest {

    @Rule
    public final SettingsActivityTestRule<AppearancePreferences> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(AppearancePreferences.class);

    private AppearancePreferences mAppearancePreferences;

    @Before
    public void setup() {
        Looper.prepare();
    }

    // Verifies that all appearance preferences appear in the correct order. When two preferences
    // share the same order value the AndroidX Preference framework falls back to alphabetical title
    // comparison, producing locale-dependent ordering. This test catches regressions in English;
    // the test infrastructure does not support switching the app locale to reproduce e.g. French
    // or Portuguese failures directly.
    @Test
    @SmallTest
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testAppearancePreferencesOrder() {
        startSettings();

        // All keys from applyOrdering() listed in expected display order. Preferences that are
        // conditionally absent (e.g. address_bar on older devices, toolbar_shortcut when adaptive
        // toolbar is disabled, show_brave_rewards_icon when Rewards is unsupported) are skipped in
        // the comparison when null.
        final String[] sortedPrefKeys = {
            AppearanceSettingsFragment.PREF_UI_THEME,
            AppearancePreferences.PREF_BRAVE_CUSTOMIZE_MENU,
            AppearanceSettingsFragment.PREF_TOOLBAR_SHORTCUT,
            AppearanceSettingsFragment.PREF_BOOKMARK_BAR,
            AppearancePreferences.PREF_ADDRESS_BAR,
            BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
            AppearancePreferences.PREF_SHOW_BRAVE_REWARDS_ICON,
            AppearancePreferences.PREF_ADS_SWITCH,
            AppearancePreferences.PREF_BRAVE_NIGHT_MODE_ENABLED,
            AppearancePreferences.PREF_BRAVE_DISABLE_SHARING_HUB,
            AppearancePreferences.PREF_BRAVE_ENABLE_TAB_GROUPS,
            AppearancePreferences.PREF_ENABLE_MULTI_WINDOWS,
            AppearancePreferences.PREF_SHOW_UNDO_WHEN_TABS_CLOSED,
        };

        @Nullable Preference prevPref = null;
        for (String key : sortedPrefKeys) {
            Preference pref = mAppearancePreferences.findPreference(key);
            if (pref == null) continue;
            if (prevPref == null) {
                prevPref = pref;
                continue;
            }
            assertTrue(
                    "\"" + prevPref.getTitle() + "\" should precede \"" + pref.getTitle() + "\"",
                    pref.getOrder() > prevPref.getOrder());
            prevPref = pref;
        }
    }

    @Test
    @SmallTest
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testLegacyTabRowsShownWhenTabGroupSettingsFeatureDisabled() {
        startSettings();

        Preference enableTabGroupsPreference =
                mAppearancePreferences.findPreference(
                        AppearancePreferences.PREF_BRAVE_ENABLE_TAB_GROUPS);
        Preference showUndoWhenTabsClosedPreference =
                mAppearancePreferences.findPreference(
                        AppearancePreferences.PREF_SHOW_UNDO_WHEN_TABS_CLOSED);

        Assert.assertNotNull(enableTabGroupsPreference);
        Assert.assertNotNull(showUndoWhenTabsClosedPreference);
        assertTrue(enableTabGroupsPreference.isVisible());
        assertTrue(showUndoWhenTabsClosedPreference.isVisible());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testLegacyTabRowsHiddenWhenTabGroupSettingsFeatureEnabled() {
        startSettings();

        Assert.assertNull(
                mAppearancePreferences
                        .getPreferenceScreen()
                        .findPreference(AppearancePreferences.PREF_BRAVE_ENABLE_TAB_GROUPS));
        Assert.assertNull(
                mAppearancePreferences
                        .getPreferenceScreen()
                        .findPreference(AppearancePreferences.PREF_SHOW_UNDO_WHEN_TABS_CLOSED));
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mAppearancePreferences = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mAppearancePreferences);
    }
}

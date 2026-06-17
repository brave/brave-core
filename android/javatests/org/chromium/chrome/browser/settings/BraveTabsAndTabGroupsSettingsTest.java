/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.os.Looper;

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
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/** Tests for {@link BraveTabsAndTabGroupsSettings}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Tests cannot run batched because they launch a Settings activity.")
public class BraveTabsAndTabGroupsSettingsTest {
    @Rule
    public final SettingsActivityTestRule<BraveTabsAndTabGroupsSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveTabsAndTabGroupsSettings.class);

    private BraveTabsAndTabGroupsSettings mSettings;

    @Before
    public void setup() {
        Looper.prepare();
        resetPrefsToDefaults();
    }

    @After
    public void tearDown() {
        resetPrefsToDefaults();
    }

    @Test
    @SmallTest
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testBraveAndroidTabGroupsSettingsDisabled() {
        assertFalse(BraveTabsAndTabGroupsSettings.isBraveAndroidTabGroupsSettingsEnabled());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testBraveAndroidTabGroupsSettingsEnabled() {
        assertTrue(BraveTabsAndTabGroupsSettings.isBraveAndroidTabGroupsSettingsEnabled());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testTabGroupsParentDisablesDependentRowsInPlace() {
        startSettings();

        ChromeSwitchPreference enableTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_ENABLE_TAB_GROUPS_SWITCH);
        Preference autoOpenSyncedTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH);
        ChromeSwitchPreference tabGroupsBarSwitch =
                mSettings.findPreference(BraveTabsAndTabGroupsSettings.PREF_TAB_GROUPS_BAR_SWITCH);
        ChromeSwitchPreference openLinksInCurrentTabGroupSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_OPEN_LINKS_IN_CURRENT_TAB_GROUP_SWITCH);
        Preference archiveSettings =
                mSettings.findPreference(BraveTabsAndTabGroupsSettings.PREF_TAB_ARCHIVE_SETTINGS);
        ChromeSwitchPreference closingAllTabsClosesBraveSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_CLOSING_ALL_TABS_CLOSES_BRAVE);
        ChromeSwitchPreference showUndoWhenTabsClosedSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_SHOW_UNDO_WHEN_TABS_CLOSED);

        assertNotNull(enableTabGroupsSwitch);
        assertNotNull(autoOpenSyncedTabGroupsSwitch);
        assertNotNull(tabGroupsBarSwitch);
        assertNotNull(openLinksInCurrentTabGroupSwitch);
        assertNotNull(archiveSettings);
        assertNotNull(closingAllTabsClosesBraveSwitch);
        assertNotNull(showUndoWhenTabsClosedSwitch);

        assertTrue(enableTabGroupsSwitch.isChecked());
        assertTrue(tabGroupsBarSwitch.isEnabled());
        assertTrue(openLinksInCurrentTabGroupSwitch.isEnabled());

        enableTabGroupsSwitch.onClick();

        assertFalse(enableTabGroupsSwitch.isChecked());
        if (autoOpenSyncedTabGroupsSwitch.isVisible()) {
            assertFalse(autoOpenSyncedTabGroupsSwitch.isEnabled());
        }
        assertTrue(tabGroupsBarSwitch.isVisible());
        assertFalse(tabGroupsBarSwitch.isEnabled());
        assertTrue(tabGroupsBarSwitch.isChecked());
        assertTrue(openLinksInCurrentTabGroupSwitch.isVisible());
        assertFalse(openLinksInCurrentTabGroupSwitch.isEnabled());
        assertTrue(openLinksInCurrentTabGroupSwitch.isChecked());
        assertTrue(archiveSettings.isEnabled());
        assertTrue(closingAllTabsClosesBraveSwitch.isEnabled());
        assertTrue(showUndoWhenTabsClosedSwitch.isEnabled());

        enableTabGroupsSwitch.onClick();

        assertTrue(enableTabGroupsSwitch.isChecked());
        if (autoOpenSyncedTabGroupsSwitch.isVisible()) {
            assertTrue(autoOpenSyncedTabGroupsSwitch.isEnabled());
        }
        assertTrue(tabGroupsBarSwitch.isEnabled());
        assertTrue(openLinksInCurrentTabGroupSwitch.isEnabled());
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mSettings = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mSettings);
    }

    private void resetPrefsToDefaults() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED, true);
        CloseBraveManager.setClosingAllTabsClosesBraveEnabled(false);
    }
}

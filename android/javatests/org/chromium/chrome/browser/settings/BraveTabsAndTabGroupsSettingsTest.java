/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

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
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.user_prefs.UserPrefs;

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

        ThreadUtils.runOnUiThreadBlocking(enableTabGroupsSwitch::onClick);

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

        ThreadUtils.runOnUiThreadBlocking(enableTabGroupsSwitch::onClick);

        assertTrue(enableTabGroupsSwitch.isChecked());
        if (autoOpenSyncedTabGroupsSwitch.isVisible()) {
            assertTrue(autoOpenSyncedTabGroupsSwitch.isEnabled());
        }
        assertTrue(tabGroupsBarSwitch.isEnabled());
        assertTrue(openLinksInCurrentTabGroupSwitch.isEnabled());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testAutoOpenSyncedChoiceSavedWhenDisabledAndRestoredWhenEnabled() {
        startSettings();

        ChromeSwitchPreference enableTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_ENABLE_TAB_GROUPS_SWITCH);
        ChromeSwitchPreference autoOpenSyncedTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH);
        assertNotNull(enableTabGroupsSwitch);
        assertNotNull(autoOpenSyncedTabGroupsSwitch);

        // The save/restore behavior only applies when auto-open is configurable, i.e. tab group
        // sync is enabled for the profile. Skip the test otherwise.
        assumeTrue(autoOpenSyncedTabGroupsSwitch.isVisible());

        // Put the user's auto-open choice into a known "on" state while tab groups are enabled.
        // This is done via the switch so it doesn't depend on the initial native pref value.
        setSwitchChecked(autoOpenSyncedTabGroupsSwitch, true);
        assertTrue(autoOpenSyncedTabGroupsSwitch.isChecked());
        assertTrue(readAutoOpenSyncedUserChoicePref());
        assertTrue(readNativeAutoOpenSyncedPref());

        // Disabling the master switch saves the user's choice but forces the native pref off.
        ThreadUtils.runOnUiThreadBlocking(enableTabGroupsSwitch::onClick);
        assertFalse(enableTabGroupsSwitch.isChecked());
        assertTrue(readAutoOpenSyncedUserChoicePref());
        assertFalse(readNativeAutoOpenSyncedPref());

        // Re-enabling the master switch restores the native pref from the saved choice.
        ThreadUtils.runOnUiThreadBlocking(enableTabGroupsSwitch::onClick);
        assertTrue(enableTabGroupsSwitch.isChecked());
        assertTrue(readAutoOpenSyncedUserChoicePref());
        assertTrue(readNativeAutoOpenSyncedPref());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testAutoOpenSyncedEffectivePrefReconciledWithoutOpeningSettings() {
        startSettings();

        ChromeSwitchPreference autoOpenSyncedTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH);
        assertNotNull(autoOpenSyncedTabGroupsSwitch);
        assumeTrue(autoOpenSyncedTabGroupsSwitch.isVisible());

        // User wants auto-open on while tab groups are enabled -> effective pref on.
        setSwitchChecked(autoOpenSyncedTabGroupsSwitch, true);
        assertTrue(readAutoOpenSyncedUserChoicePref());
        assertTrue(readNativeAutoOpenSyncedPref());

        // Simulate the master becoming disabled WITHOUT going through the settings toggle (e.g. a
        // change to its default value), then the startup reconcile running. The effective pref must
        // be forced off while the user's choice is preserved.
        setMasterEnabledDirectlyAndReconcile(false);
        assertFalse(readNativeAutoOpenSyncedPref());
        assertTrue(readAutoOpenSyncedUserChoicePref());

        // Re-enabling the master (again without the settings toggle) restores the effective pref.
        setMasterEnabledDirectlyAndReconcile(true);
        assertTrue(readNativeAutoOpenSyncedPref());
        assertTrue(readAutoOpenSyncedUserChoicePref());
    }

    private void startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        mSettings = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", mSettings);
    }

    /**
     * Writes the master pref directly (bypassing the settings toggle) and runs the same
     * reconciliation the startup path uses.
     */
    private void setMasterEnabledDirectlyAndReconcile(boolean enabled) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    ChromeSharedPreferences.getInstance()
                            .writeBoolean(
                                    BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
                    BraveTabUiFeatureUtilities.applyAutoOpenSyncedTabGroupsEffectivePref(
                            mSettings.getProfile());
                });
    }

    private static void setSwitchChecked(ChromeSwitchPreference preference, boolean checked) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    if (preference.isChecked() != checked) {
                        preference.onClick();
                    }
                });
    }

    private static boolean readAutoOpenSyncedUserChoicePref() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_AUTO_OPEN_SYNCED_TAB_GROUPS_USER_CHOICE, false);
    }

    private boolean readNativeAutoOpenSyncedPref() {
        return ThreadUtils.runOnUiThreadBlocking(
                () ->
                        UserPrefs.get(mSettings.getProfile())
                                .getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS));
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
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_AUTO_OPEN_SYNCED_TAB_GROUPS_USER_CHOICE);
        CloseBraveManager.setClosingAllTabsClosesBraveEnabled(false);
    }
}

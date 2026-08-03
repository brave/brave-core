/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
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
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modelutil.PropertyModel;

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
        // All test methods share the instrumentation thread, so only the first one prepares it.
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
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
        // Groups are kept when they are synced, so the switch flips without a confirmation.
        BraveTabsAndTabGroupsSettings.setTabGroupSyncActiveForTesting(true);
        startSettings();

        ChromeSwitchPreference enableTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_ENABLE_TAB_GROUPS_SWITCH);
        Preference showSyncedTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_SHOW_SYNCED_TAB_GROUPS_SWITCH);
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
        assertNotNull(showSyncedTabGroupsSwitch);
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
        // "Show synced tab groups" is independent of the master switch, so it stays usable.
        assertTrue(showSyncedTabGroupsSwitch.isEnabled());
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
        assertTrue(showSyncedTabGroupsSwitch.isEnabled());
        assertTrue(tabGroupsBarSwitch.isEnabled());
        assertTrue(openLinksInCurrentTabGroupSwitch.isEnabled());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testCancellingConfirmationKeepsTabGroupsEnabled() {
        BraveTabsAndTabGroupsSettings.setTabGroupSyncActiveForTesting(false);
        startSettings();

        ChromeSwitchPreference enableTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_ENABLE_TAB_GROUPS_SWITCH);
        ChromeSwitchPreference tabGroupsBarSwitch =
                mSettings.findPreference(BraveTabsAndTabGroupsSettings.PREF_TAB_GROUPS_BAR_SWITCH);
        assertNotNull(enableTabGroupsSwitch);
        assertNotNull(tabGroupsBarSwitch);
        assertTrue(enableTabGroupsSwitch.isChecked());

        clickConfirmationButton(enableTabGroupsSwitch, ModalDialogProperties.ButtonType.NEGATIVE);

        assertTrue(enableTabGroupsSwitch.isChecked());
        assertTrue(isTabGroupsEnabled());
        assertTrue(tabGroupsBarSwitch.isEnabled());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testAcceptingConfirmationDisablesTabGroups() {
        BraveTabsAndTabGroupsSettings.setTabGroupSyncActiveForTesting(false);
        startSettings();

        ChromeSwitchPreference enableTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_ENABLE_TAB_GROUPS_SWITCH);
        ChromeSwitchPreference tabGroupsBarSwitch =
                mSettings.findPreference(BraveTabsAndTabGroupsSettings.PREF_TAB_GROUPS_BAR_SWITCH);
        assertNotNull(enableTabGroupsSwitch);
        assertNotNull(tabGroupsBarSwitch);
        assertTrue(enableTabGroupsSwitch.isChecked());

        clickConfirmationButton(enableTabGroupsSwitch, ModalDialogProperties.ButtonType.POSITIVE);

        assertFalse(enableTabGroupsSwitch.isChecked());
        assertFalse(isTabGroupsEnabled());
        assertFalse(tabGroupsBarSwitch.isEnabled());
    }

    @Test
    @SmallTest
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testNoConfirmationWhenTabGroupsAreSynced() {
        BraveTabsAndTabGroupsSettings.setTabGroupSyncActiveForTesting(true);
        startSettings();

        ChromeSwitchPreference enableTabGroupsSwitch =
                mSettings.findPreference(
                        BraveTabsAndTabGroupsSettings.PREF_ENABLE_TAB_GROUPS_SWITCH);
        assertNotNull(enableTabGroupsSwitch);

        ThreadUtils.runOnUiThreadBlocking(enableTabGroupsSwitch::onClick);

        assertNull(getModalDialogManager().getCurrentDialogForTest());
        assertFalse(enableTabGroupsSwitch.isChecked());
        assertFalse(isTabGroupsEnabled());
    }

    /** Turns the tab groups switch off and answers the confirmation dialog with {@code button}. */
    private void clickConfirmationButton(
            ChromeSwitchPreference enableTabGroupsSwitch,
            @ModalDialogProperties.ButtonType int button) {
        ModalDialogManager modalDialogManager = getModalDialogManager();
        ThreadUtils.runOnUiThreadBlocking(enableTabGroupsSwitch::onClick);

        CriteriaHelper.pollUiThread(
                () -> {
                    PropertyModel dialog = modalDialogManager.getCurrentDialogForTest();
                    if (dialog == null) {
                        return false;
                    }
                    dialog.get(ModalDialogProperties.CONTROLLER).onClick(dialog, button);
                    return true;
                },
                "The disable tab groups confirmation dialog was never shown.");
        CriteriaHelper.pollUiThread(
                () -> modalDialogManager.getCurrentDialogForTest() == null,
                "The disable tab groups confirmation dialog was never dismissed.");
    }

    private static boolean isTabGroupsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
    }

    private ModalDialogManager getModalDialogManager() {
        ModalDialogManager modalDialogManager =
                mSettingsActivityTestRule.getActivity().getModalDialogManager();
        assertNotNull(modalDialogManager);
        return modalDialogManager;
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

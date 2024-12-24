/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;

import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.browser.sync.settings.BraveManageSyncSettings;
import org.chromium.chrome.browser.sync.settings.ManageSyncSettings;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/** Tests for BraveManageSyncSettings. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DoNotBatch(reason = "TODO(crbug.com/40743432): SyncTestRule doesn't support batching.")
public class BraveManageSyncSettingsTest {
    private SettingsActivity mSettingsActivity;

    private final SettingsActivityTestRule<BraveManageSyncSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveManageSyncSettings.class);

    @Before
    public void setUp() {}

    @Test
    @SmallTest
    @Feature({"Sync"})
    @DisableFeatures({ChromeFeatureList.REPLACE_SYNC_PROMOS_WITH_SIGN_IN_PROMOS})
    public void syncEverythingOrPasswordsHandlerIsOriginalOnChromeOS() {
        syncEverythingOrPasswordsOverridden(true, false);
    }

    @Test
    @SmallTest
    @Feature({"Sync"})
    @DisableFeatures({ChromeFeatureList.REPLACE_SYNC_PROMOS_WITH_SIGN_IN_PROMOS})
    public void syncEverythingOrPasswordsHandlerOverriddenOnNonChromeOS() {
        syncEverythingOrPasswordsOverridden(false, true);
    }

    void syncEverythingOrPasswordsOverridden(
            Boolean isChromeOS, Boolean handlerShouldBeOverridden) {
        BraveManageSyncSettings.setIsRunningOnChromeOSForTesting(isChromeOS);
        BraveManageSyncSettings fragment = startManageSyncPreferences();

        ChromeSwitchPreference prefSyncPasswords =
                fragment.findPreference(ManageSyncSettings.PREF_SYNC_PASSWORDS);
        ChromeSwitchPreference syncEverything =
                fragment.findPreference(ManageSyncSettings.PREF_SYNC_EVERYTHING);

        Preference.OnPreferenceChangeListener origSyncPasswordsListner =
                prefSyncPasswords.getOnPreferenceChangeListener();
        Preference.OnPreferenceChangeListener origSyncEverythingListner =
                syncEverything.getOnPreferenceChangeListener();

        Assert.assertEquals(
                handlerShouldBeOverridden,
                origSyncPasswordsListner != (Preference.OnPreferenceChangeListener) fragment);
        Assert.assertEquals(
                handlerShouldBeOverridden,
                origSyncEverythingListner != (Preference.OnPreferenceChangeListener) fragment);
    }

    private BraveManageSyncSettings startManageSyncPreferences() {
        mSettingsActivity = mSettingsActivityTestRule.startSettingsActivity();
        return mSettingsActivityTestRule.getFragment();
    }
}

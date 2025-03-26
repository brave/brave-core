/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;

import static org.mockito.Mockito.when;

import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.browser.sync.settings.BraveManageSyncSettings;
import org.chromium.chrome.browser.sync.settings.ManageSyncSettings;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.sync.SyncService;
import org.chromium.components.sync.TransportState;

/** Tests for BraveManageSyncSettings. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DoNotBatch(reason = "TODO(crbug.com/40743432): SyncTestRule doesn't support batching.")
public class BraveManageSyncSettingsTest {
    private SettingsActivity mSettingsActivity;

    private final SettingsActivityTestRule<BraveManageSyncSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveManageSyncSettings.class);

    @Mock private SyncService mSyncService;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    @SmallTest
    @Feature({"Sync"})
    public void syncEverythingOrPasswordsHandlerIsOriginalOnChromeOS() {
        syncEverythingOrPasswordsOverridden(true, false);
    }

    @Test
    @SmallTest
    @Feature({"Sync"})
    public void syncEverythingOrPasswordsHandlerOverriddenOnNonChromeOS() {
        syncEverythingOrPasswordsOverridden(false, true);
    }

    void syncEverythingOrPasswordsOverridden(
            Boolean isChromeOS, Boolean handlerShouldBeOverridden) {

        setupMockSyncService();

        // The next line triggers presubmit warning
        // Banned functions were used.
        // ...
        // It is safe to ignore this warning if you are just moving an existing
        // call, or if you want special handling for users in the legacy state.
        // Support the legacy state is the case for Brave Sync
        when(mSyncService.hasSyncConsent()).thenReturn(true);

        when(mSyncService.getSetupInProgressHandle())
                .thenReturn(
                        new SyncService.SyncSetupInProgressHandle() {
                            @Override
                            public void close() {}
                        });

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

    private void setupMockSyncService() {
        SyncServiceFactory.setInstanceForTesting(mSyncService);
        when(mSyncService.getTransportState()).thenReturn(TransportState.ACTIVE);
    }
}

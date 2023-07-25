/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.brave_tricks.checkbox_to_switch.ChromeBaseCheckBoxPreference;

/**
 * See org.brave.bytecode.BraveManageSyncSettingsClassAdapter
 */
public class BraveManageSyncSettings extends ManageSyncSettings {
    private static final String PREF_ADVANCED_CATEGORY = "advanced_category";

    private Preference mGoogleActivityControls;
    private Preference mSyncEncryption;

    @VisibleForTesting
    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        Preference reviewSyncData = findPreference(PREF_SYNC_REVIEW_DATA);
        assert reviewSyncData != null : "Something has changed in the upstream!";
        if (reviewSyncData != null) {
            getPreferenceScreen().removePreference(reviewSyncData);
        }

        Preference turnOffSync = findPreference(PREF_TURN_OFF_SYNC);
        assert turnOffSync != null : "Something has changed in the upstream!";
        if (turnOffSync != null) {
            getPreferenceScreen().removePreference(turnOffSync);
        }

        Preference syncReadingList = findPreference(PREF_SYNC_READING_LIST);
        assert syncReadingList != null : "Something has changed in the upstream!";
        if (syncReadingList != null) {
            syncReadingList.setVisible(false);
        }

        Preference syncAutofill = findPreference(PREF_SYNC_AUTOFILL);
        assert syncAutofill != null : "Something has changed in the upstream!";
        if (syncAutofill != null) {
            syncAutofill.setTitle(R.string.brave_sync_autofill);
        }

        getPreferenceScreen().removePreference(mGoogleActivityControls);
        getPreferenceScreen().removePreference(mSyncEncryption);

        findPreference(PREF_ADVANCED_CATEGORY).setVisible(false);

        Preference syncPaymentsIntegration = findPreference(PREF_SYNC_PAYMENTS_INTEGRATION);
        assert syncPaymentsIntegration != null : "Something has changed in the upstream!";
        if (syncPaymentsIntegration != null) {
            syncPaymentsIntegration.setVisible(false);
        }
    }
}

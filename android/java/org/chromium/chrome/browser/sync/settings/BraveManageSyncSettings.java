/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.sync.settings.ManageSyncSettings;
import org.chromium.chrome.browser.ui.brave_tricks.checkbox_to_switch.CheckBoxPreference;

// See org.brave.bytecode.BraveManageSyncSettingsClassAdapter
public class BraveManageSyncSettings extends ManageSyncSettings {
    private static final String PREF_ADVANCED_CATEGORY = "advanced_category";
    private static final String PREF_SYNC_REVIEW_DATA = "sync_review_data";
    private static final String PREF_TURN_OFF_SYNC = "turn_off_sync";

    private Preference mGoogleActivityControls;
    private Preference mSyncEncryption;

    private CheckBoxPreference mSyncPaymentsIntegration;
    private CheckBoxPreference mSyncReadingList;
    private CheckBoxPreference mSyncAutofill;

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

        getPreferenceScreen().removePreference(mGoogleActivityControls);
        getPreferenceScreen().removePreference(mSyncEncryption);

        findPreference(PREF_ADVANCED_CATEGORY).setVisible(false);

        mSyncPaymentsIntegration.setVisible(false);
        mSyncReadingList.setVisible(false);
        mSyncAutofill.setTitle(R.string.brave_sync_autofill);
    }
}

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
import org.chromium.chrome.browser.device_reauth.DeviceAuthRequester;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.components.browser_ui.settings.brave_tricks.checkbox_to_switch.ChromeBaseCheckBoxPreference;

/**
 * See org.brave.bytecode.BraveManageSyncSettingsClassAdapter
 */
public class BraveManageSyncSettings extends ManageSyncSettings {
    private static final String PREF_ADVANCED_CATEGORY = "advanced_category";

    private Preference mGoogleActivityControls;
    private Preference mSyncEncryption;

    private ChromeBaseCheckBoxPreference mSyncPaymentsIntegration;

    @Nullable
    private ReauthenticatorBridge mReauthenticatorBridge;

    private ChromeBaseCheckBoxPreference mPrefSyncPasswords;

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

        mSyncPaymentsIntegration.setVisible(false);

        mReauthenticatorBridge = ReauthenticatorBridge.create(
                DeviceAuthRequester.PAYMENT_METHODS_REAUTH_IN_SETTINGS);
        mPrefSyncPasswords = findPreference(PREF_SYNC_PASSWORDS);

        Preference.OnPreferenceChangeListener origListner =
                mPrefSyncPasswords.getOnPreferenceChangeListener();

        mPrefSyncPasswords.setOnPreferenceChangeListener((Preference preference,
                                                                 Object newValue) -> {
            if ((Boolean) newValue == false) {
                return origListner.onPreferenceChange(preference, newValue);
            } else {
                mReauthenticatorBridge.reauthenticate(success -> {
                    if (success) {
                        origListner.onPreferenceChange(preference, true); // <- not really worked

                        mPrefSyncPasswords.setChecked(true);
                    }
                }, /*useLastValidAuth=*/false);

                return false;
            }
        });
    }
}

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.Preference;

import org.chromium.chrome.browser.sync.settings.ManageSyncSettings;

// See org.brave.bytecode.BraveManageSyncSettingsClassAdapter
public class BraveManageSyncSettings extends ManageSyncSettings {
    private Preference mGoogleActivityControls;

    private Preference mSyncEncryption;

    private Preference mManageSyncData;

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        getPreferenceScreen().removePreference(mGoogleActivityControls);
        getPreferenceScreen().removePreference(mSyncEncryption);
        getPreferenceScreen().removePreference(mManageSyncData);
    }
}

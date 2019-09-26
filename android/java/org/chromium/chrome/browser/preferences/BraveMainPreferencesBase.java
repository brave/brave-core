/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import java.util.HashMap;

// This exculdes some settings in main settings screen.
public class BraveMainPreferencesBase extends PreferenceFragmentCompat {
    // Below prefs are removed from main settings.
    private static final String PREF_ACCOUNT_SECTION = "account_section";
    private static final String PREF_SIGN_IN = "sign_in";
    private static final String PREF_DATA_REDUCTION = "data_reduction";
    private static final String PREF_AUTOFILL_ASSISTANT = "autofill_assistant";
    private static final String PREF_SYNC_AND_SERVICES = "sync_and_services";
    private static final String PREF_DEVELOPER = "developer";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        updateBravePreferences();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateBravePreferences();
    }

    private void updateBravePreferences() {
        removePreferenceIfPresent(PREF_SIGN_IN);
        removePreferenceIfPresent(PREF_ACCOUNT_SECTION);
        removePreferenceIfPresent(PREF_DATA_REDUCTION);
        removePreferenceIfPresent(PREF_AUTOFILL_ASSISTANT);
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES);
        removePreferenceIfPresent(PREF_DEVELOPER);
    }

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public Preference findPreference(CharSequence key) {
        Preference result = super.findPreference(key);
        if (result == null) {
            result = mRemovedPreferences.get(key);
        }
        return result;
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.privacy;

import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeBaseCheckBoxPreferenceCompat;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.chrome.browser.preferences.privacy.PrivacyPreferences;

public class BravePrivacyPreferences extends PrivacyPreferences {
    private static final String PREF_HTTPSE = "httpse";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        PreferenceUtils.addPreferencesFromResource(this, R.xml.brave_privacy_preferences);

        super.onCreatePreferences(savedInstanceState, rootKey);

        ChromeBaseCheckBoxPreferenceCompat httpsePref =
                (ChromeBaseCheckBoxPreferenceCompat) findPreference(PREF_HTTPSE);
        httpsePref.setOnPreferenceChangeListener(this);

        ChromeBaseCheckBoxPreferenceCompat adBlockPref =
                (ChromeBaseCheckBoxPreferenceCompat) findPreference(PREF_AD_BLOCK);
        adBlockPref.setOnPreferenceChangeListener(this);

        ChromeBaseCheckBoxPreferenceCompat fingerprintingProtectionPref =
                (ChromeBaseCheckBoxPreferenceCompat) findPreference(PREF_FINGERPRINTING_PROTECTION);
        fingerprintingProtectionPref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        super.onPreferenceChange(preference, newValue);

        String key = preference.getKey();
        if (PREF_HTTPSE.equals(key)) {
            BravePrefServiceBridge.getInstance().setHTTPSEEnabled((boolean) newValue);
        } else if (PREF_AD_BLOCK.equals(key)) {
            BravePrefServiceBridge.getInstance().setAdBlockEnabled((boolean) newValue);
        } else if (PREF_FINGERPRINTING_PROTECTION.equals(key)) {
            BravePrefServiceBridge.getInstance().setFingerprintingProtectionEnabled(
                    (boolean) newValue);
        }

        return true;
    }
}

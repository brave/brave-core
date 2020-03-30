/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.settings.ChromeBaseCheckBoxPreference;
import org.chromium.chrome.browser.settings.ChromeSwitchPreference;
import org.chromium.chrome.browser.settings.ManagedPreferenceDelegate;
import org.chromium.chrome.browser.settings.privacy.PrivacySettings;
import org.chromium.chrome.browser.settings.SettingsUtils;

public class BravePrivacySettings extends PrivacySettings {
    private static final String PREF_HTTPSE = "httpse";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_SYNC_AND_SERVICES_LINK = "sync_and_services_link";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_CLEAR_BROWSING_DATA = "clear_browsing_data";

    private final PrefServiceBridge mPrefServiceBridge = PrefServiceBridge.getInstance();
    private final ManagedPreferenceDelegate mManagedPreferenceDelegate =
            createManagedPreferenceDelegate();
    private ChromeSwitchPreference mSearchSuggestions;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_privacy_preferences);

        super.onCreatePreferences(savedInstanceState, rootKey);

        ChromeBaseCheckBoxPreference httpsePref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_HTTPSE);
        httpsePref.setOnPreferenceChangeListener(this);

        ChromeBaseCheckBoxPreference adBlockPref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_AD_BLOCK);
        adBlockPref.setOnPreferenceChangeListener(this);

        ChromeBaseCheckBoxPreference fingerprintingProtectionPref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_FINGERPRINTING_PROTECTION);
        fingerprintingProtectionPref.setOnPreferenceChangeListener(this);

        ChromeBaseCheckBoxPreference closeTabsOnExitPref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_CLOSE_TABS_ON_EXIT);
        closeTabsOnExitPref.setOnPreferenceChangeListener(this);

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        updatePreferences();
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
        } else if (PREF_CLOSE_TABS_ON_EXIT.equals(key)) {
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(PREF_CLOSE_TABS_ON_EXIT, (boolean) newValue);
            sharedPreferencesEditor.apply();
        } else if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
            mPrefServiceBridge.setBoolean(Pref.SEARCH_SUGGEST_ENABLED, (boolean) newValue);
        }

        return true;
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferences();
    }

    private void updatePreferences() {
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES_LINK);
        mSearchSuggestions.setChecked(mPrefServiceBridge.getBoolean(Pref.SEARCH_SUGGEST_ENABLED));
        mSearchSuggestions.setOrder(findPreference(PREF_CLEAR_BROWSING_DATA).getOrder() + 1);
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
    }

    private ManagedPreferenceDelegate createManagedPreferenceDelegate() {
        return preference -> {
            String key = preference.getKey();
            if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
                return mPrefServiceBridge.isManagedPreference(Pref.SEARCH_SUGGEST_ENABLED);
            }
            return false;
        };
    }
}

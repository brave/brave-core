/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.privacy.settings.PrivacySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.components.browser_ui.settings.ChromeBaseCheckBoxPreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

public class BravePrivacySettings extends PrivacySettings {
    private static final String PREF_HTTPSE = "httpse";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_SYNC_AND_SERVICES_LINK = "sync_and_services_link";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_AUTOCOMPLETE_TOP_SITES = "autocomplete_top_sites";
    private static final String PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES = "autocomplete_brave_suggested_sites";
    private static final String PREF_CLEAR_BROWSING_DATA = "clear_browsing_data";

    private final PrefService mPrefServiceBridge = UserPrefs.get(Profile.getLastUsedRegularProfile());
    private final ChromeManagedPreferenceDelegate mManagedPreferenceDelegate =
            createManagedPreferenceDelegate();
    private ChromeSwitchPreference mSearchSuggestions;
    private ChromeSwitchPreference mAutocompleteTopSites;
    private ChromeSwitchPreference mAutocompleteBraveSuggestedSites;

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

        mAutocompleteTopSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_TOP_SITES);
        mAutocompleteTopSites.setOnPreferenceChangeListener(this);

        mAutocompleteBraveSuggestedSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES);
        mAutocompleteBraveSuggestedSites.setOnPreferenceChangeListener(this);

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
        } else if (PREF_AUTOCOMPLETE_TOP_SITES.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED, (boolean) newValue);
        } else if (PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(BravePref.BRAVE_SUGGESTED_SITE_SUGGESTIONS_ENABLED,
                    (boolean) newValue);
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
        mAutocompleteTopSites
                .setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED));
        mAutocompleteTopSites.setOrder(findPreference(PREF_CLEAR_BROWSING_DATA).getOrder() + 2);
        mAutocompleteBraveSuggestedSites.setChecked(
                UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.BRAVE_SUGGESTED_SITE_SUGGESTIONS_ENABLED));
        mAutocompleteBraveSuggestedSites.setOrder(findPreference(PREF_CLEAR_BROWSING_DATA).getOrder() + 3);
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
    }

    private ChromeManagedPreferenceDelegate createManagedPreferenceDelegate() {
        return preference -> {
            String key = preference.getKey();
            if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
                return mPrefServiceBridge.isManagedPreference(Pref.SEARCH_SUGGEST_ENABLED);
            }
            return false;
        };
    }
}

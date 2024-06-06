/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.os.Handler;

import androidx.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveSearchEnginesPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String PREF_STANDARD_SEARCH_ENGINE = "standard_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "private_search_engine";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";

    private ChromeManagedPreferenceDelegate mManagedPreferenceDelegate;
    private ChromeSwitchPreference mSearchSuggestions;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.brave_search_engines);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_search_engines_preferences);
    }

    @Override
    public void onResume() {
        super.onResume();
        new Handler().post(() -> updateSearchEnginePreference());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mManagedPreferenceDelegate = createManagedPreferenceDelegate();
    }

    private ChromeManagedPreferenceDelegate createManagedPreferenceDelegate() {
        return new ChromeManagedPreferenceDelegate(getProfile()) {
            @Override
            public boolean isPreferenceControlledByPolicy(Preference preference) {
                String key = preference.getKey();
                if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
                    return UserPrefs.get(getProfile())
                            .isManagedPreference(Pref.SEARCH_SUGGEST_ENABLED);
                }
                return false;
            }
        };
    }

    private void updateSearchEnginePreference() {
        Profile lastUsedRegularProfile = ProfileManager.getLastUsedRegularProfile();
        Preference searchEnginePreference = findPreference(PREF_STANDARD_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(
                BraveSearchEngineUtils.getDSEShortName(lastUsedRegularProfile, false));

        searchEnginePreference = findPreference(PREF_PRIVATE_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(
                BraveSearchEngineUtils.getDSEShortName(
                        lastUsedRegularProfile.getPrimaryOTRProfile(/* createIfNeeded= */ true),
                        true));

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setChecked(
                UserPrefs.get(getProfile()).getBoolean(Pref.SEARCH_SUGGEST_ENABLED));
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
            UserPrefs.get(getProfile()).setBoolean(Pref.SEARCH_SUGGEST_ENABLED, (boolean) newValue);
        }
        return true;
    }
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.os.Handler;

import androidx.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveSearchEnginesPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String PREF_STANDARD_SEARCH_ENGINE = "standard_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "private_search_engine";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR =
            "show_autocomplete_in_address_bar";
    private static final String PREF_AUTOCOMPLETE_TOP_SITES = "autocomplete_top_sites";
    private static final String PREF_ADD_OPEN_SEARCH_ENGINES = "brave.other_search_engines_enabled";

    private ChromeManagedPreferenceDelegate mManagedPreferenceDelegate;

    private ChromeSwitchPreference mShowAutocompleteInAddressBar;
    private ChromeSwitchPreference mSearchSuggestions;
    private ChromeSwitchPreference mAutocompleteTopSites;
    private ChromeSwitchPreference mAddOpenSearchEngines;

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
        Preference searchEnginePreference = findPreference(PREF_STANDARD_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(
                BraveSearchEngineUtils.getDSEShortName(getProfile(), false));

        searchEnginePreference = findPreference(PREF_PRIVATE_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(
                BraveSearchEngineUtils.getDSEShortName(
                        getProfile().getPrimaryOTRProfile(/* createIfNeeded= */ true), true));

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        mShowAutocompleteInAddressBar =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR);
        mShowAutocompleteInAddressBar.setOnPreferenceChangeListener(this);

        mAutocompleteTopSites =
                (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_TOP_SITES);
        mAutocompleteTopSites.setOnPreferenceChangeListener(this);

        boolean autocompleteEnabled =
                UserPrefs.get(getProfile()).getBoolean(BravePref.AUTOCOMPLETE_ENABLED);
        mSearchSuggestions.setVisible(autocompleteEnabled);
        mAutocompleteTopSites.setVisible(autocompleteEnabled);

        mShowAutocompleteInAddressBar.setChecked(autocompleteEnabled);
        mSearchSuggestions.setChecked(
                UserPrefs.get(getProfile()).getBoolean(Pref.SEARCH_SUGGEST_ENABLED));
        mAutocompleteTopSites.setChecked(
                UserPrefs.get(getProfile()).getBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED));

        mAddOpenSearchEngines =
                (ChromeSwitchPreference) findPreference(PREF_ADD_OPEN_SEARCH_ENGINES);
        mAddOpenSearchEngines.setOnPreferenceChangeListener(this);
        mAddOpenSearchEngines.setChecked(
                UserPrefs.get(getProfile()).getBoolean(BravePref.ADD_OPEN_SEARCH_ENGINES));
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
            UserPrefs.get(getProfile()).setBoolean(Pref.SEARCH_SUGGEST_ENABLED, (boolean) newValue);
        } else if (PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR.equals(key)) {
            boolean autocompleteEnabled = (boolean) newValue;
            mSearchSuggestions.setVisible(autocompleteEnabled);
            mAutocompleteTopSites.setVisible(autocompleteEnabled);
            UserPrefs.get(getProfile())
                    .setBoolean(BravePref.AUTOCOMPLETE_ENABLED, autocompleteEnabled);
        } else if (PREF_AUTOCOMPLETE_TOP_SITES.equals(key)) {
            UserPrefs.get(getProfile())
                    .setBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED, (boolean) newValue);
        } else if (PREF_ADD_OPEN_SEARCH_ENGINES.equals(key)) {
            UserPrefs.get(getProfile())
                    .setBoolean(BravePref.ADD_OPEN_SEARCH_ENGINES, (boolean) newValue);
        }
        return true;
    }
}

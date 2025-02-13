/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;

import androidx.preference.Preference;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.web_discovery.WebDiscoveryPrefs;

public class BraveSearchEnginesPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String PREF_STANDARD_SEARCH_ENGINE = "standard_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "private_search_engine";

    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR =
            "show_autocomplete_in_address_bar";
    private static final String PREF_AUTOCOMPLETE_TOP_SUGGESTIONS = "autocomplete_top_sites";
    private static final String PREF_ADD_OPEN_SEARCH_ENGINES = "brave.other_search_engines_enabled";
    private static final String PREF_SEND_WEB_DISCOVERY = "send_web_discovery";

    private static final String PREF_CUSTOM_SEARCH_ENGINES = "pref_custom_search_engines";

    private ChromeManagedPreferenceDelegate mManagedPreferenceDelegate;

    private ChromeSwitchPreference mShowAutocompleteInAddressBar;
    private ChromeSwitchPreference mSearchSuggestions;
    private ChromeSwitchPreference mAutocompleteTopSuggestions;
    private ChromeSwitchPreference mAddOpenSearchEngines;
    private ChromeSwitchPreference mSendWebDiscovery;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.brave_search_engines));
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_search_engines_preferences);
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
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

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
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

    @SuppressLint("NotifyDataSetChanged")
    private void updateSearchEnginePreference() {
        Preference searchEnginePreference = findPreference(PREF_STANDARD_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(
                BraveSearchEngineUtils.getDSEShortName(getProfile(), false));

        searchEnginePreference = findPreference(PREF_PRIVATE_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(
                BraveSearchEngineUtils.getDSEShortName(
                        getProfile().getPrimaryOtrProfile(/* createIfNeeded= */ true), true));

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        mShowAutocompleteInAddressBar =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR);
        mShowAutocompleteInAddressBar.setOnPreferenceChangeListener(this);

        mAutocompleteTopSuggestions =
                (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_TOP_SUGGESTIONS);
        mAutocompleteTopSuggestions.setOnPreferenceChangeListener(this);

        boolean autocompleteEnabled =
                UserPrefs.get(getProfile()).getBoolean(BravePref.AUTOCOMPLETE_ENABLED);
        mSearchSuggestions.setVisible(autocompleteEnabled);
        mAutocompleteTopSuggestions.setVisible(autocompleteEnabled);

        mShowAutocompleteInAddressBar.setChecked(autocompleteEnabled);
        mSearchSuggestions.setChecked(
                UserPrefs.get(getProfile()).getBoolean(Pref.SEARCH_SUGGEST_ENABLED));
        mAutocompleteTopSuggestions.setChecked(
                UserPrefs.get(getProfile()).getBoolean(BravePref.TOP_SUGGESTIONS_ENABLED));

        mAddOpenSearchEngines =
                (ChromeSwitchPreference) findPreference(PREF_ADD_OPEN_SEARCH_ENGINES);
        mAddOpenSearchEngines.setOnPreferenceChangeListener(this);
        mAddOpenSearchEngines.setChecked(
                UserPrefs.get(getProfile()).getBoolean(BravePref.ADD_OPEN_SEARCH_ENGINES));

        if (BraveConfig.WEB_DISCOVERY_ENABLED) {
            mSendWebDiscovery = (ChromeSwitchPreference) findPreference(PREF_SEND_WEB_DISCOVERY);
            mSendWebDiscovery.setOnPreferenceChangeListener(this);
        } else {
            removePreferenceIfPresent(PREF_SEND_WEB_DISCOVERY);
        }

        if (mSendWebDiscovery != null) {
            mSendWebDiscovery.setTitle(
                    getActivity().getResources().getString(R.string.send_web_discovery_title));
            mSendWebDiscovery.setSummary(
                    getActivity().getResources().getString(R.string.send_web_discovery_summary));
            mSendWebDiscovery.setChecked(
                    UserPrefs.get(getProfile())
                            .getBoolean(WebDiscoveryPrefs.WEB_DISCOVERY_ENABLED));
        }
        // Preference customSearchEnginePreference =
        // getPreferenceScreen().findPreference(PREF_CUSTOM_SEARCH_ENGINES);
        // if (customSearchEnginePreference != null) {
        //     getPreferenceScreen().removePreference(customSearchEnginePreference);
        // }

        // Preference customSearchEnginePreference2 =
        // getPreferenceScreen().findPreference(PREF_CUSTOM_SEARCH_ENGINES);
        // if (customSearchEnginePreference2 != null) {
        //     getPreferenceScreen().addPreference(customSearchEnginePreference2);
        // }

        Preference customPreference = findPreference(PREF_CUSTOM_SEARCH_ENGINES);
        if (customPreference
                instanceof
                org.chromium.brave.browser.search_engines.settings.CustomSearchEnginesPreference) {
            View view = getView();
            if (view != null) {
                RecyclerView recyclerView = view.findViewById(R.id.custom_search_engine_list);
                RecyclerView.Adapter<?> adapter = recyclerView.getAdapter();
                if (adapter != null) {
                    adapter.notifyDataSetChanged(); // Refresh UI
                }
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
            UserPrefs.get(getProfile()).setBoolean(Pref.SEARCH_SUGGEST_ENABLED, (boolean) newValue);
        } else if (PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR.equals(key)) {
            boolean autocompleteEnabled = (boolean) newValue;
            mSearchSuggestions.setVisible(autocompleteEnabled);
            mAutocompleteTopSuggestions.setVisible(autocompleteEnabled);
            UserPrefs.get(getProfile())
                    .setBoolean(BravePref.AUTOCOMPLETE_ENABLED, autocompleteEnabled);
        } else if (PREF_AUTOCOMPLETE_TOP_SUGGESTIONS.equals(key)) {
            UserPrefs.get(getProfile())
                    .setBoolean(BravePref.TOP_SUGGESTIONS_ENABLED, (boolean) newValue);
        } else if (PREF_ADD_OPEN_SEARCH_ENGINES.equals(key)) {
            UserPrefs.get(getProfile())
                    .setBoolean(BravePref.ADD_OPEN_SEARCH_ENGINES, (boolean) newValue);
        } else if (PREF_SEND_WEB_DISCOVERY.equals(key)) {
            UserPrefs.get(getProfile())
                    .setBoolean(WebDiscoveryPrefs.WEB_DISCOVERY_ENABLED, (boolean) newValue);
        }
        return true;
    }
}

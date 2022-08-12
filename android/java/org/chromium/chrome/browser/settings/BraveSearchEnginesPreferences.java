/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.os.Handler;

import androidx.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.privacy.settings.PrivacySettings;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.search_engines.TemplateUrlService;

public class BraveSearchEnginesPreferences
        extends BravePreferenceFragment implements TemplateUrlService.TemplateUrlServiceObserver {
    private static final String PREF_STANDARD_SEARCH_ENGINE = "standard_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "private_search_engine";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.brave_search_engines);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_search_engines_preferences);
        TemplateUrlServiceFactory.get().addObserver(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        TemplateUrlServiceFactory.get().removeObserver(this);
    }

    @Override
    public void onResume() {
        super.onResume();
        new Handler().post(() -> updateSearchEnginePreference());
    }

    private void updateSearchEnginePreference() {
        Preference searchEnginePreference = findPreference(PREF_STANDARD_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(BraveSearchEngineUtils.getDSEShortName(false));

        searchEnginePreference = findPreference(PREF_PRIVATE_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(BraveSearchEngineUtils.getDSEShortName(true));
    }

    @Override
    public void onTemplateURLServiceChanged() {
        new Handler().post(() -> updateSearchEnginePreference());
    }
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.HashMap;

// This exculdes some settings in main settings screen.
public class BraveMainPreferencesBase extends PreferenceFragmentCompat {
    private static final String PREF_STANDARD_SEARCH_ENGINE = "standard_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "private_search_engine";
    private static final String PREF_SEARCH_ENGINE_SECTION = "search_engine_section";
    private static final String PREF_BACKGROUND_VIDEO_PLAYBACK = "background_video_playback";
    private static final String PREF_CLOSING_ALL_TABS_CLOSES_BRAVE = "closing_all_tabs_closes_brave";
    private static final String PREF_ADVANCED_SECTION = "advanced_section";
    private static final String PREF_PRIVACY = "privacy";
    private static final String PREF_ACCESSIBILITY = "accessibility";
    private static final String PREF_CONTENT_SETTINGS = "content_settings";
    private static final String PREF_ABOUT_CHROME = "about_chrome";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (MainPreference::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        PreferenceUtils.addPreferencesFromResource(this, R.xml.brave_main_preferences);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    @Override
    public void onResume() {
        super.onResume();
        // Run updateBravePreferences() after fininshing MainPreferences::updatePreferences().
        // Otherwise, some prefs could be added after finishing updateBravePreferences().
        new Handler().post(() -> updateBravePreferences());
    }

    private void updateBravePreferences() {
        // Below prefs are removed from main settings.
        removePreferenceIfPresent(MainPreferences.PREF_SIGN_IN);
        removePreferenceIfPresent(MainPreferences.PREF_ACCOUNT_SECTION);
        removePreferenceIfPresent(MainPreferences.PREF_DATA_REDUCTION);
        removePreferenceIfPresent(MainPreferences.PREF_AUTOFILL_ASSISTANT);
        removePreferenceIfPresent(MainPreferences.PREF_SYNC_AND_SERVICES);
        removePreferenceIfPresent(MainPreferences.PREF_DEVELOPER);
        removePreferenceIfPresent(MainPreferences.PREF_SEARCH_ENGINE);

        updateSearchEnginePreference();
        updateControlSectionPreferences();

        rearrangePreferenceOrders();
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

    /**
     * Re-arrange by resetting each preference's order.
     * With this, we can insert our own preferences at any position.
     */
    private void rearrangePreferenceOrders() {
        // We don't need to consider search engine section because they are using 0 ~ 2 ordered
        // and we deleted original 0 ~ 2 ordered preferences.
        // Advanced section will be located below our controls section.
        int order = findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE).getOrder();
        findPreference(PREF_ADVANCED_SECTION).setOrder(++order);
        findPreference(PREF_PRIVACY).setOrder(++order);
        findPreference(PREF_ACCESSIBILITY).setOrder(++order);
        findPreference(PREF_CONTENT_SETTINGS).setOrder(++order);
        findPreference(MainPreferences.PREF_LANGUAGES).setOrder(++order);
        findPreference(MainPreferences.PREF_DATA_REDUCTION).setOrder(++order);
        findPreference(MainPreferences.PREF_DOWNLOADS).setOrder(++order);
        findPreference(MainPreferences.PREF_DEVELOPER).setOrder(++order);
        findPreference(PREF_ABOUT_CHROME).setOrder(++order);
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }

    private void updateSearchEnginePreference() {
        if (!TemplateUrlServiceFactory.get().isLoaded()) {
            ChromeBasePreferenceCompat searchEnginePref =
                    (ChromeBasePreferenceCompat) findPreference(PREF_SEARCH_ENGINE_SECTION);
            searchEnginePref.setEnabled(false);
            return;
        }

        Preference searchEnginePreference = findPreference(PREF_STANDARD_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(BraveSearchEngineUtils.getDSEShortName(false));

        searchEnginePreference = findPreference(PREF_PRIVATE_SEARCH_ENGINE);
        searchEnginePreference.setEnabled(true);
        searchEnginePreference.setSummary(BraveSearchEngineUtils.getDSEShortName(true));
    }

    void updateControlSectionPreferences() {
        // TODO(simonhong): Update summary properly.
        Preference p = findPreference(PREF_BACKGROUND_VIDEO_PLAYBACK);
        p.setSummary(BackgroundVideoPlaybackPreference.getPreferenceSummary());
        p = findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE);
        p.setSummary(R.string.prefs_closing_all_tabs_closes_brave_summary_enabled);
    }
}

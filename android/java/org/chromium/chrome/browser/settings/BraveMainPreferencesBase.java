/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AlertDialog;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;
import android.util.DisplayMetrics;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.privacy.BravePrivacyPreferences;
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
    private static final String PREF_SYNC = "brave_sync_layout";
    private static final String PREF_ACCESSIBILITY = "accessibility";
    private static final String PREF_CONTENT_SETTINGS = "content_settings";
    private static final String PREF_ABOUT_CHROME = "about_chrome";
    private static final String PREF_WELCOME_TOUR = "welcome_tour";
    private static final String PREF_BRAVE_REWARDS = "brave_rewards";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (MainPreference::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_main_preferences);

        overrideChromiumPreferences();
        initWelcomeTourPreference();
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
        removePreferenceIfPresent(MainPreferences.PREF_UI_THEME);

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
        findPreference(PREF_BRAVE_REWARDS).setOrder(++order);
        findPreference(PREF_SYNC).setOrder(++order);
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
            ChromeBasePreference searchEnginePref =
                    (ChromeBasePreference) findPreference(PREF_SEARCH_ENGINE_SECTION);
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

    private void updateControlSectionPreferences() {
        Preference p = findPreference(PREF_BACKGROUND_VIDEO_PLAYBACK);
        p.setSummary(BackgroundVideoPlaybackPreference.getPreferenceSummary());
        p = findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE);
        p.setSummary(ClosingAllTabsClosesBravePreference.getPreferenceSummary());
    }

    private void overrideChromiumPreferences() {
        // Replace fragment.
        findPreference(PREF_PRIVACY).setFragment(BravePrivacyPreferences.class.getName());
    }

    private void initWelcomeTourPreference() {
        findPreference(PREF_WELCOME_TOUR).setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                final Context context = preference.getContext();
                final TextView titleTextView = new TextView (context);
                titleTextView.setText(context.getResources().getString(R.string.welcome_tour_dialog_text));
                int padding = dp2px(20);
                titleTextView.setPadding(padding, padding, padding, padding);
                titleTextView.setTextSize(18);
                titleTextView.setTextColor(context.getResources().getColor(R.color.standard_mode_tint));
                titleTextView.setTypeface(null, Typeface.BOLD);

                AlertDialog alertDialog = new AlertDialog.Builder(context, R.style.Theme_Chromium_AlertDialog)
                    .setView(titleTextView)
                    .setPositiveButton(R.string.continue_button, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            OnboardingPrefManager.getInstance().showOnboarding(context, true);
                        }
                    })
                    .setNegativeButton(android.R.string.cancel, null)
                    .create();
                alertDialog.show();
                return true;
            }
        });
    }

    // TODO(simonhong): Make this static public with proper class.
    private int dp2px(int dp) {
        final float DP_PER_INCH_MDPI = 160f;
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float px = dp * (metrics.densityDpi / DP_PER_INCH_MDPI);
        return Math.round(px);
    }
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.AlertDialog;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;

import androidx.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.homepage.settings.BraveHomepageSettings;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.rate.RateDialogFragment;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.settings.BraveStatsPreferences;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.HashMap;

// This exculdes some settings in main settings screen.
public class BraveMainPreferencesBase extends BravePreferenceFragment {
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
    private static final String PREF_BACKGROUND_IMAGES = "backgroud_images";
    private static final String PREF_BRAVE_REWARDS = "brave_rewards";
    private static final String PREF_HOMEPAGE = "homepage";
    private static final String PREF_USE_CUSTOM_TABS = "use_custom_tabs";
    private static final String PREF_LANGUAGES = "languages";
    private static final String PREF_BRAVE_LANGUAGES = "brave_languages";
    private static final String PREF_RATE_BRAVE = "rate_brave";
    private static final String PREF_BRAVE_STATS = "brave_stats";
    private static final String PREF_BRAVE_DOWNLOADS = "brave_downloads";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (MainPreference::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_main_preferences);

        overrideChromiumPreferences();
        initRateBrave();
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
        removePreferenceIfPresent(MainSettings.PREF_SYNC_PROMO);
        removePreferenceIfPresent(MainSettings.PREF_SIGN_IN);
        removePreferenceIfPresent(MainSettings.PREF_ACCOUNT_SECTION);
        removePreferenceIfPresent(MainSettings.PREF_DATA_REDUCTION);
        removePreferenceIfPresent(MainSettings.PREF_SYNC_AND_SERVICES);
        removePreferenceIfPresent(MainSettings.PREF_SEARCH_ENGINE);
        removePreferenceIfPresent(MainSettings.PREF_UI_THEME);
        removePreferenceIfPresent(MainSettings.PREF_DOWNLOADS);
        removePreferenceIfPresent(MainSettings.PREF_SAFETY_CHECK);
        removePreferenceIfPresent(PREF_LANGUAGES);

        updateSearchEnginePreference();
        updateControlSectionPreferences();

        rearrangePreferenceOrders();

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS) ||
                BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            removePreferenceIfPresent(PREF_BRAVE_REWARDS);
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M 
            || (NTPUtil.isReferralEnabled() && NTPBackgroundImagesBridge.enableSponsoredImages())) {
            removePreferenceIfPresent(PREF_BACKGROUND_IMAGES);
        }
    }

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public <T extends Preference> T findPreference(CharSequence key) {
        T result = super.findPreference(key);
        if (result == null) {
            result = (T) mRemovedPreferences.get((String) key);
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
        if (DeviceFormFactor.isTablet()) {
            removePreferenceIfPresent(PREF_USE_CUSTOM_TABS);
        } else {
            findPreference(PREF_USE_CUSTOM_TABS).setOrder(++order);
        }
        findPreference(PREF_ADVANCED_SECTION).setOrder(++order);
        findPreference(PREF_PRIVACY).setOrder(++order);
        findPreference(PREF_BRAVE_REWARDS).setOrder(++order);
        findPreference(PREF_SYNC).setOrder(++order);
        findPreference(PREF_ACCESSIBILITY).setOrder(++order);
        findPreference(PREF_CONTENT_SETTINGS).setOrder(++order);
        findPreference(PREF_BRAVE_LANGUAGES).setOrder(++order);
        findPreference(MainSettings.PREF_DATA_REDUCTION).setOrder(++order);
        findPreference(PREF_BRAVE_DOWNLOADS).setOrder(++order);
        // This preference doesn't exist by default in Release mode
        if (findPreference(MainSettings.PREF_DEVELOPER) != null) {
            findPreference(MainSettings.PREF_DEVELOPER).setOrder(++order);
        }
        findPreference(PREF_ABOUT_CHROME).setOrder(++order);

        // If gn flag enable_brave_sync is false, hide Sync pref
        if (BraveConfig.SYNC_ENABLED == false) {
            removePreferenceIfPresent(PREF_SYNC);
        }

        // We don't have home button on top toolbar at the moment
        if (!DeviceFormFactor.isTablet() && !BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            removePreferenceIfPresent(PREF_HOMEPAGE);
        }
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
        p = findPreference(PREF_USE_CUSTOM_TABS);
        p.setSummary(BraveCustomTabsPreference.getPreferenceSummary());
        p = findPreference(PREF_BRAVE_STATS);
        p.setSummary(BraveStatsPreferences.getPreferenceSummary());
    }

    private void overrideChromiumPreferences() {
        // Replace fragment.
        findPreference(PREF_PRIVACY).setFragment(BravePrivacySettings.class.getName());
        findPreference(PREF_HOMEPAGE).setFragment(BraveHomepageSettings.class.getName());
    }

    private void initRateBrave() {
        findPreference(PREF_RATE_BRAVE).setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                Bundle bundle = new Bundle();
                bundle.putBoolean(RateUtils.FROM_SETTINGS, true);

                RateDialogFragment mRateDialogFragment = new RateDialogFragment();
                mRateDialogFragment.setCancelable(false);
                mRateDialogFragment.setArguments(bundle);
                mRateDialogFragment.show(getParentFragmentManager(), "RateDialogFragment");
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

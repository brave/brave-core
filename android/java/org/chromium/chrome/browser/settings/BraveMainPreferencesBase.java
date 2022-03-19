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

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.homepage.settings.BraveHomepageSettings;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.rate.RateDialogFragment;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.settings.BraveStatsPreferences;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.HashMap;

// This exculdes some settings in main settings screen.
public class BraveMainPreferencesBase
        extends BravePreferenceFragment implements Preference.OnPreferenceChangeListener {
    // sections
    private static final String PREF_FEATURES_SECTION = "features_section";
    private static final String PREF_DISPLAY_SECTION = "display_section";
    private static final String PREF_GENERAL_SECTION = "general_section";
    private static final String PREF_BASICS_SECTION = "basics_section";
    private static final String PREF_ADVANCED_SECTION = "advanced_section";
    private static final String PREF_ONLINE_CHECKOUT_SECTION = "online_checkout_section";
    private static final String PREF_SEARCH_ENGINE_SECTION = "search_engine_section";
    private static final String PREF_SUPPORT_SECTION = "support_section";
    private static final String PREF_ABOUT_SECTION = "about_section";

    // prefs

    private static final String PREF_BRAVE_VPN_CALLOUT = "pref_vpn_callout";
    private static final String PREF_STANDARD_SEARCH_ENGINE = "standard_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "private_search_engine";
    private static final String PREF_BACKGROUND_VIDEO_PLAYBACK = "background_video_playback";
    private static final String PREF_CLOSING_ALL_TABS_CLOSES_BRAVE = "closing_all_tabs_closes_brave";
    private static final String PREF_PRIVACY = "privacy";
    private static final String PREF_SHIELDS_AND_PRIVACY = "brave_shields_and_privacy";
    private static final String PREF_BRAVE_SEARCH_ENGINES = "brave_search_engines";
    private static final String PREF_BRAVE_NEWS = "brave_news";
    private static final String PREF_SYNC = "brave_sync_layout";
    private static final String PREF_PASSWORDS = "passwords";
    private static final String PREF_NOTIFICATIONS = "notifications";
    private static final String PREF_PAYMENT_METHODS = "autofill_payment_methods";
    private static final String PREF_ADDRESSES = "autofill_addresses";
    private static final String PREF_APPEARANCE = "appearance";
    private static final String PREF_NEW_TAB_PAGE = "background_images";
    private static final String PREF_ACCESSIBILITY = "accessibility";
    private static final String PREF_CONTENT_SETTINGS = "content_settings";
    private static final String PREF_ABOUT_CHROME = "about_chrome";
    private static final String PREF_BACKGROUND_IMAGES = "backgroud_images";
    private static final String PREF_BRAVE_REWARDS = "brave_rewards";
    private static final String PREF_BRAVE_WALLET = "brave_wallet";
    private static final String PREF_BRAVE_VPN = "brave_vpn";
    private static final String PREF_HOMEPAGE = "homepage";
    private static final String PREF_USE_CUSTOM_TABS = "use_custom_tabs";
    private static final String PREF_LANGUAGES = "languages";
    private static final String PREF_BRAVE_LANGUAGES = "brave_languages";
    private static final String PREF_RATE_BRAVE = "rate_brave";
    private static final String PREF_BRAVE_STATS = "brave_stats";
    private static final String PREF_DOWNLOADS = "brave_downloads";

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
        setPreferenceListeners();
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
        removePreferenceIfPresent(MainSettings.PREF_SEARCH_ENGINE);
        removePreferenceIfPresent(MainSettings.PREF_UI_THEME);
        removePreferenceIfPresent(MainSettings.PREF_DOWNLOADS);
        removePreferenceIfPresent(MainSettings.PREF_SAFETY_CHECK);
        removePreferenceIfPresent(MainSettings.PREF_ACCOUNT_AND_GOOGLE_SERVICES_SECTION);
        removePreferenceIfPresent(MainSettings.PREF_GOOGLE_SERVICES);
        removePreferenceIfPresent(PREF_LANGUAGES);
        removePreferenceIfPresent(PREF_BASICS_SECTION);
        // removePreferenceIfPresent(PREF_HOMEPAGE);

        // removePreferenceIfPresent(PREF_USE_CUSTOM_TABS);
        removePreferenceIfPresent(PREF_ADVANCED_SECTION);
        removePreferenceIfPresent(PREF_PRIVACY);
        removePreferenceIfPresent(PREF_BRAVE_VPN_CALLOUT);
        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
            removePreferenceIfPresent(PREF_BRAVE_NEWS);
        }
        updateSearchEnginePreference();

        updateSummaries();

        // updates the icons - normally the ones from Chromium
        updatePreferenceIcons();
        // rearanges programmatically the order for the prefs from Brave and Chromium
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
        int firstSectionOrder = 0;

        if (BraveVpnPrefUtils.shouldShowCallout() && !BraveVpnPrefUtils.isSubscriptionPurchase()
                && BraveVpnUtils.isBraveVpnFeatureEnable()) {
            VpnCalloutPreference vpnCalloutPreference = new VpnCalloutPreference(getActivity());
            vpnCalloutPreference.setKey(PREF_BRAVE_VPN_CALLOUT);
            vpnCalloutPreference.setOrder(firstSectionOrder);
            getPreferenceScreen().addPreference(vpnCalloutPreference);
        }

        findPreference(PREF_FEATURES_SECTION).setOrder(++firstSectionOrder);

        findPreference(PREF_SHIELDS_AND_PRIVACY).setOrder(++firstSectionOrder);
        findPreference(PREF_BRAVE_REWARDS).setOrder(++firstSectionOrder);

        if (ChromeFeatureList.isEnabled(BraveFeatureList.NATIVE_BRAVE_WALLET)) {
            findPreference(PREF_BRAVE_WALLET).setOrder(++firstSectionOrder);
        } else {
            removePreferenceIfPresent(PREF_BRAVE_WALLET);
        }

        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            findPreference(PREF_BRAVE_VPN).setOrder(++firstSectionOrder);
        } else {
            removePreferenceIfPresent(PREF_BRAVE_VPN);
        }

        int generalOrder = firstSectionOrder;
        findPreference(PREF_GENERAL_SECTION).setOrder(++generalOrder);

        findPreference(PREF_BRAVE_SEARCH_ENGINES).setOrder(++generalOrder);
        findPreference(PREF_HOMEPAGE).setOrder(++generalOrder);
        findPreference(PREF_PASSWORDS).setOrder(++generalOrder);
        findPreference(PREF_SYNC).setOrder(++generalOrder);
        findPreference(PREF_BRAVE_STATS).setOrder(++generalOrder);
        // if notification is not available (eg. for emulators)
        if (findPreference(PREF_NOTIFICATIONS) != null) {
            findPreference(PREF_NOTIFICATIONS).setOrder(++generalOrder);
        }
        findPreference(PREF_CONTENT_SETTINGS).setOrder(++generalOrder);
        findPreference(PREF_DOWNLOADS).setOrder(++generalOrder);
        findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE).setOrder(++generalOrder);
        if (DeviceFormFactor.isTablet()) {
            removePreferenceIfPresent(PREF_USE_CUSTOM_TABS);
        } else {
            findPreference(PREF_USE_CUSTOM_TABS).setOrder(++generalOrder);
        }
        findPreference(PREF_BACKGROUND_VIDEO_PLAYBACK).setOrder(++generalOrder);

        int displaySectionOrder = generalOrder;
        findPreference(PREF_DISPLAY_SECTION).setOrder(++displaySectionOrder);

        findPreference(PREF_APPEARANCE).setOrder(++displaySectionOrder);
        findPreference(PREF_NEW_TAB_PAGE).setOrder(++displaySectionOrder);
        findPreference(PREF_ACCESSIBILITY).setOrder(++displaySectionOrder);
        findPreference(PREF_BRAVE_LANGUAGES).setOrder(++displaySectionOrder);

        int onlineCheckoutSectionOrder = displaySectionOrder;
        findPreference(PREF_ONLINE_CHECKOUT_SECTION).setOrder(++onlineCheckoutSectionOrder);

        findPreference(PREF_PAYMENT_METHODS).setOrder(++onlineCheckoutSectionOrder);
        findPreference(PREF_ADDRESSES).setOrder(++onlineCheckoutSectionOrder);

        int supportSectionOrder = onlineCheckoutSectionOrder;
        findPreference(PREF_SUPPORT_SECTION).setOrder(++supportSectionOrder);

        findPreference(PREF_RATE_BRAVE).setOrder(++supportSectionOrder);

        int aboutSectionOrder = supportSectionOrder;
        // This preference doesn't exist by default in Release mode
        if (findPreference(MainSettings.PREF_DEVELOPER) != null) {
            findPreference(MainSettings.PREF_DEVELOPER).setOrder(++aboutSectionOrder);
        }
        findPreference(PREF_ABOUT_SECTION).setOrder(++aboutSectionOrder);

        // This preference doesn't exist by default in Release mode
        if (findPreference(MainSettings.PREF_DEVELOPER) != null) {
            findPreference(MainSettings.PREF_DEVELOPER).setOrder(++aboutSectionOrder);
        }
        findPreference(PREF_ABOUT_CHROME).setOrder(++aboutSectionOrder);

        int order = findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE).getOrder();

        // We don't have home button on top toolbar at the moment
        if (!DeviceFormFactor.isTablet() && !BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            removePreferenceIfPresent(PREF_HOMEPAGE);
        }
    }

    private void updatePreferenceIcon(String preferenceString, int drawable) {
        Preference preference = findPreference(preferenceString);
        if (preference != null) {
            preference.setIcon(drawable);
        }
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }

    private void updatePreferenceIcons() {
        updatePreferenceIcon(PREF_PASSWORDS, R.drawable.ic_password);
        updatePreferenceIcon(PREF_CONTENT_SETTINGS, R.drawable.ic_site_settings);
        updatePreferenceIcon(PREF_PAYMENT_METHODS, R.drawable.ic_payment_methods);
        updatePreferenceIcon(PREF_DOWNLOADS, R.drawable.ic_downloads);
        updatePreferenceIcon(PREF_LANGUAGES, R.drawable.ic_languages);
        updatePreferenceIcon(PREF_BRAVE_LANGUAGES, R.drawable.ic_languages);
        updatePreferenceIcon(PREF_ABOUT_CHROME, R.drawable.ic_info);
        updatePreferenceIcon(PREF_ACCESSIBILITY, R.drawable.ic_accessibility);
        updatePreferenceIcon(PREF_PRIVACY, R.drawable.ic_privacy_reports);
        updatePreferenceIcon(PREF_ADDRESSES, R.drawable.ic_addresses);
        updatePreferenceIcon(PREF_NOTIFICATIONS, R.drawable.ic_notification);
        updatePreferenceIcon(MainSettings.PREF_DEVELOPER, R.drawable.ic_info);
        updatePreferenceIcon(PREF_HOMEPAGE, R.drawable.ic_homepage);
    }

    private void updateSearchEnginePreference() {
        if (!TemplateUrlServiceFactory.get().isLoaded()) {
            ChromeBasePreference searchEnginePref =
                    (ChromeBasePreference) findPreference(PREF_BRAVE_SEARCH_ENGINES);
            searchEnginePref.setEnabled(false);
            return;
        }
    }

    private void updateSummary(String preferenceString, int summary) {
        Preference p = findPreference(preferenceString);
        p.setSummary(summary);
    }

    private void updateSummaries() {
        updateSummary(PREF_USE_CUSTOM_TABS, BraveCustomTabsPreference.getPreferenceSummary());
        updateSummary(PREF_BRAVE_STATS, BraveStatsPreferences.getPreferenceSummary());
        if (BravePrefServiceBridge.getInstance().getBackgroundVideoPlaybackEnabled()) {
            updateSummary(
                    PREF_BACKGROUND_VIDEO_PLAYBACK, R.string.prefs_background_video_playback_on);
        }
    }

    private void overrideChromiumPreferences() {
        // Replace fragment.
        findPreference(PREF_SHIELDS_AND_PRIVACY).setFragment(BravePrivacySettings.class.getName());
        findPreference(PREF_HOMEPAGE).setFragment(BraveHomepageSettings.class.getName());
    }

    private void setPreferenceListeners() {
        findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE).setOnPreferenceChangeListener(this);
        findPreference(PREF_BACKGROUND_VIDEO_PLAYBACK).setOnPreferenceChangeListener(this);
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

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_CLOSING_ALL_TABS_CLOSES_BRAVE.equals(key)) {
            CloseBraveManager.setClosingAllTabsClosesBraveEnabled((boolean) newValue);
        }

        if (PREF_BACKGROUND_VIDEO_PLAYBACK.equals(key)) {
            BravePrefServiceBridge.getInstance().setBackgroundVideoPlaybackEnabled(
                    (boolean) newValue);
            if ((boolean) newValue) {
                updateSummary(PREF_BACKGROUND_VIDEO_PLAYBACK,
                        R.string.prefs_background_video_playback_on);
            } else {
                findPreference(PREF_BACKGROUND_VIDEO_PLAYBACK).setSummary("");
            }
            BraveRelaunchUtils.askForRelaunch(this.getActivity());
        }

        return true;
    }
}

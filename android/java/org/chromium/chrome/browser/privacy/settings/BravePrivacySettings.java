/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.privacy.settings.PrivacySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveDialogPreference;
import org.chromium.chrome.browser.settings.BravePreferenceDialogFragment;
import org.chromium.chrome.browser.settings.BraveWebrtcPolicyPreference;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.components.browser_ui.settings.ChromeBaseCheckBoxPreference;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

public class BravePrivacySettings extends PrivacySettings {
    // Chromium Prefs
    private static final String PREF_CAN_MAKE_PAYMENT = "can_make_payment";
    private static final String PREF_NETWORK_PREDICTIONS = "preload_pages";
    private static final String PREF_SECURE_DNS = "secure_dns";
    private static final String PREF_USAGE_STATS = "usage_stats_reporting";
    private static final String PREF_DO_NOT_TRACK = "do_not_track";
    private static final String PREF_SAFE_BROWSING = "safe_browsing";
    private static final String PREF_SYNC_AND_SERVICES_LINK = "sync_and_services_link";
    private static final String PREF_CLEAR_BROWSING_DATA = "clear_browsing_data";
    private static final String PREF_PRIVACY_SANDBOX = "privacy_sandbox";

    // brave Prefs
    private static final String PREF_BRAVE_SHIELDS_GLOBALS_SECTION =
            "brave_shields_globals_section";
    private static final String PREF_CLEAR_DATA_SECTION = "clear_data_section";
    private static final String PREF_BRAVE_SOCIAL_BLOCKING_SECTION =
            "brave_social_blocking_section";
    private static final String PREF_OTHER_PRIVACY_SETTINGS_SECTION =
            "other_privacy_settings_section";

    private static final String PREF_HTTPSE = "httpse";
    private static final String PREF_IPFS_GATEWAY = "ipfs_gateway";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_BLOCK_SCRIPTS = "scripts_block";
    private static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_HTTPS_EVERYWHERE = "https_everywhere";
    private static final String PREF_SEND_P3A = "send_p3a_analytics";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_AUTOCOMPLETE_TOP_SITES = "autocomplete_top_sites";
    private static final String PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES = "autocomplete_brave_suggested_sites";
    private static final String PREF_SOCIAL_BLOCKING_GOOGLE = "social_blocking_google";
    private static final String PREF_SOCIAL_BLOCKING_FACEBOOK = "social_blocking_facebook";
    private static final String PREF_SOCIAL_BLOCKING_TWITTER = "social_blocking_twitter";
    private static final String PREF_SOCIAL_BLOCKING_LINKEDIN = "social_blocking_linkedin";
    private static final String PREF_WEBRTC_POLICY = "webrtc_policy";
    private static final String PREF_UNSTOPPABLE_DOMAINS = "unstoppable_domains";
    private static final String PREF_ETH_NAMED_SERVICE = "ens";

    private static final String PREF_BLOCK_TRACKERS_ADS = "block_trackers_ads";
    private static final String PREF_BLOCK_CROSS_SITE_COOKIES = "block_cross_site_cookies";
    private static final String PREF_SHIELDS_SUMMARY = "shields_summary";
    private static final String PREF_CLEAR_ON_EXIT = "clear_on_exit";

    private static final String[] NEW_PRIVACY_PREFERENCE_ORDER = {
            PREF_BRAVE_SHIELDS_GLOBALS_SECTION, //  shields globals  section
            PREF_SHIELDS_SUMMARY, PREF_BLOCK_TRACKERS_ADS, PREF_HTTPSE, PREF_BLOCK_SCRIPTS,
            PREF_BLOCK_CROSS_SITE_COOKIES, PREF_FINGERPRINTING_PROTECTION,
            PREF_CLEAR_DATA_SECTION, //  clear data automatically  section
            PREF_CLEAR_ON_EXIT, PREF_CLEAR_BROWSING_DATA,
            PREF_BRAVE_SOCIAL_BLOCKING_SECTION, // social blocking section
            PREF_SOCIAL_BLOCKING_GOOGLE, PREF_SOCIAL_BLOCKING_FACEBOOK,
            PREF_SOCIAL_BLOCKING_TWITTER, PREF_SOCIAL_BLOCKING_LINKEDIN,
            PREF_OTHER_PRIVACY_SETTINGS_SECTION, // other section
            PREF_WEBRTC_POLICY, PREF_SAFE_BROWSING, PREF_CAN_MAKE_PAYMENT, PREF_NETWORK_PREDICTIONS,
            PREF_UNSTOPPABLE_DOMAINS, PREF_ETH_NAMED_SERVICE, PREF_IPFS_GATEWAY, PREF_SECURE_DNS,
            PREF_DO_NOT_TRACK, PREF_CLOSE_TABS_ON_EXIT, PREF_SEND_P3A, PREF_SEARCH_SUGGESTIONS,
            PREF_AUTOCOMPLETE_TOP_SITES, PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES, PREF_USAGE_STATS,
            PREF_PRIVACY_SANDBOX

    };

    private final PrefService mPrefServiceBridge = UserPrefs.get(Profile.getLastUsedRegularProfile());
    private final ChromeManagedPreferenceDelegate mManagedPreferenceDelegate =
            createManagedPreferenceDelegate();
    private ChromeSwitchPreference mSearchSuggestions;
    private ChromeSwitchPreference mCanMakePayment;
    private BraveDialogPreference mAdsTrakersBlockPref;
    private BraveDialogPreference mBlockCrosssiteCookies;
    private ChromeSwitchPreference mAutocompleteTopSites;
    private ChromeSwitchPreference mAutocompleteBraveSuggestedSites;
    private ChromeSwitchPreference mHttpsePref;
    private BraveDialogPreference mFingerprintingProtectionPref;
    private ChromeSwitchPreference mBlockScriptsPref;
    private ChromeSwitchPreference mCloseTabsOnExitPref;
    private ChromeSwitchPreference mSendP3A;
    private ChromeSwitchPreference mIpfsGatewayPref;
    private PreferenceCategory mSocialBlockingCategory;
    private ChromeSwitchPreference mSocialBlockingGoogle;
    private ChromeSwitchPreference mHttpsEverywhere;
    private ChromeSwitchPreference mSocialBlockingFacebook;
    private ChromeSwitchPreference mSocialBlockingTwitter;
    private ChromeSwitchPreference mSocialBlockingLinkedin;
    private ChromeBasePreference mWebrtcPolicy;
    private ChromeSwitchPreference mClearBrowsingDataOnExit;
    private Preference mUstoppableDomains;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        // override title
        getActivity().setTitle(R.string.brave_shields_and_privacy);

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_privacy_preferences);

        mHttpsePref = (ChromeSwitchPreference) findPreference(PREF_HTTPSE);
        mHttpsePref.setOnPreferenceChangeListener(this);

        mCanMakePayment = (ChromeSwitchPreference) findPreference(PREF_CAN_MAKE_PAYMENT);
        mCanMakePayment.setOnPreferenceChangeListener(this);

        mAdsTrakersBlockPref = (BraveDialogPreference) findPreference(PREF_BLOCK_TRACKERS_ADS);
        mAdsTrakersBlockPref.setOnPreferenceChangeListener(this);

        mUstoppableDomains = (Preference) findPreference(PREF_UNSTOPPABLE_DOMAINS);
        mUstoppableDomains.setOnPreferenceChangeListener(this);

        mBlockCrosssiteCookies =
                (BraveDialogPreference) findPreference(PREF_BLOCK_CROSS_SITE_COOKIES);
        mBlockCrosssiteCookies.setOnPreferenceChangeListener(this);

        mBlockScriptsPref = (ChromeSwitchPreference) findPreference(PREF_BLOCK_SCRIPTS);
        mBlockScriptsPref.setOnPreferenceChangeListener(this);

        mFingerprintingProtectionPref =
                (BraveDialogPreference) findPreference(PREF_FINGERPRINTING_PROTECTION);
        mFingerprintingProtectionPref.setOnPreferenceChangeListener(this);

        mCloseTabsOnExitPref = (ChromeSwitchPreference) findPreference(PREF_CLOSE_TABS_ON_EXIT);
        mCloseTabsOnExitPref.setOnPreferenceChangeListener(this);

        mClearBrowsingDataOnExit = (ChromeSwitchPreference) findPreference(PREF_CLEAR_ON_EXIT);
        mClearBrowsingDataOnExit.setOnPreferenceChangeListener(this);

        mSendP3A = (ChromeSwitchPreference) findPreference(PREF_SEND_P3A);
        mSendP3A.setOnPreferenceChangeListener(this);

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        mAutocompleteTopSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_TOP_SITES);
        mAutocompleteTopSites.setOnPreferenceChangeListener(this);

        mAutocompleteBraveSuggestedSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES);
        mAutocompleteBraveSuggestedSites.setOnPreferenceChangeListener(this);

        mSocialBlockingCategory =
                (PreferenceCategory) findPreference(PREF_BRAVE_SOCIAL_BLOCKING_SECTION);
        mSocialBlockingCategory.setOnPreferenceChangeListener(this);

        mSocialBlockingGoogle = (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_GOOGLE);
        mSocialBlockingGoogle.setOnPreferenceChangeListener(this);

        mSocialBlockingFacebook = (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_FACEBOOK);
        mSocialBlockingFacebook.setOnPreferenceChangeListener(this);

        mSocialBlockingTwitter = (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_TWITTER);
        mSocialBlockingTwitter.setOnPreferenceChangeListener(this);

        mSocialBlockingLinkedin = (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_LINKEDIN);
        mSocialBlockingLinkedin.setOnPreferenceChangeListener(this);

        mWebrtcPolicy = (ChromeBasePreference) findPreference(PREF_WEBRTC_POLICY);

        updatePreferences();

        removePreferenceIfPresent(PREF_AD_BLOCK);
        // removePreferenceIfPresent(PREF_IPFS_GATEWAY);
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES_LINK);
        // removePreferenceIfPresent(PREF_UNSTOPPABLE_DOMAINS);
    }

    // used for displaying BraveDialogPreference
    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        if (preference instanceof BraveDialogPreference) {
            BravePreferenceDialogFragment dialogFragment =
                    BravePreferenceDialogFragment.newInstance((BraveDialogPreference) preference);
            dialogFragment.setTargetFragment(this, 0);
            if (getFragmentManager() != null) {
                dialogFragment.show(getFragmentManager(), BravePreferenceDialogFragment.TAG);
                dialogFragment.setPreferenceDialogListener(this);
            }
        } else {
            super.onDisplayPreferenceDialog(preference);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        super.onPreferenceChange(preference, newValue);
        String key = preference.getKey();

        if (PREF_HTTPSE.equals(key)) {
            BravePrefServiceBridge.getInstance().setHTTPSEEnabled((boolean) newValue);
        } else if (PREF_IPFS_GATEWAY.equals(key)) {
            BravePrefServiceBridge.getInstance().setIpfsGatewayEnabled((boolean) newValue);
        } else if (PREF_FINGERPRINTING_PROTECTION.equals(key)) {
            if ((int) newValue == 0) {
                BravePrefServiceBridge.getInstance().setFingerprintingControlType(
                        BraveShieldsContentSettings.BLOCK_RESOURCE);
                mFingerprintingProtectionPref.setSummary(getActivity().getResources().getString(
                        R.string.block_fingerprinting_option_1));
                mFingerprintingProtectionPref.setCheckedIndex(0);
            } else if ((int) newValue == 1) {
                BravePrefServiceBridge.getInstance().setFingerprintingControlType(
                        BraveShieldsContentSettings.DEFAULT);
                mFingerprintingProtectionPref.setSummary(getActivity().getResources().getString(
                        R.string.block_fingerprinting_option_2));
                mFingerprintingProtectionPref.setCheckedIndex(1);
            } else {
                BravePrefServiceBridge.getInstance().setFingerprintingControlType(
                        BraveShieldsContentSettings.ALLOW_RESOURCE);
                mFingerprintingProtectionPref.setSummary(getActivity().getResources().getString(
                        R.string.block_fingerprinting_option_3));
                mFingerprintingProtectionPref.setCheckedIndex(2);
            }
        } else if (PREF_BLOCK_CROSS_SITE_COOKIES.equals(key)) {
            if ((int) newValue == 0) {
                BravePrefServiceBridge.getInstance().setCookiesBlockType(
                        BraveShieldsContentSettings.BLOCK_RESOURCE);
                mBlockCrosssiteCookies.setSummary(
                        getActivity().getResources().getString(R.string.block_cookies_option_1));
                mBlockCrosssiteCookies.setCheckedIndex(0);
            } else if ((int) newValue == 1) {
                BravePrefServiceBridge.getInstance().setCookiesBlockType(
                        BraveShieldsContentSettings.DEFAULT);
                mBlockCrosssiteCookies.setSummary(
                        getActivity().getResources().getString(R.string.block_cross_site_cookies));
                mBlockCrosssiteCookies.setCheckedIndex(1);
            } else {
                BravePrefServiceBridge.getInstance().setCookiesBlockType(
                        BraveShieldsContentSettings.ALLOW_RESOURCE);
                mBlockCrosssiteCookies.setSummary(
                        getActivity().getResources().getString(R.string.block_cookies_option_3));
                mBlockCrosssiteCookies.setCheckedIndex(2);
            }
        } else if (PREF_BLOCK_SCRIPTS.equals(key)) {
            String settingString =
                    ((boolean) newValue ? BraveShieldsContentSettings.BLOCK_RESOURCE
                                        : BraveShieldsContentSettings.ALLOW_RESOURCE);
            BravePrefServiceBridge.getInstance().setNoScriptControlType(settingString);
        } else if (PREF_CLOSE_TABS_ON_EXIT.equals(key)) {
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(PREF_CLOSE_TABS_ON_EXIT, (boolean) newValue);
            sharedPreferencesEditor.apply();
        } else if (PREF_SEND_P3A.equals(key)) {
            BravePrefServiceBridge.getInstance().setP3AEnabled((boolean) newValue);
        } else if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
            mPrefServiceBridge.setBoolean(Pref.SEARCH_SUGGEST_ENABLED, (boolean) newValue);
        } else if (PREF_AUTOCOMPLETE_TOP_SITES.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED, (boolean) newValue);
        } else if (PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(BravePref.BRAVE_SUGGESTED_SITE_SUGGESTIONS_ENABLED,
                    (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_GOOGLE.equals(key)) {
            BravePrefServiceBridge.getInstance().setThirdPartyGoogleLoginEnabled(
                    (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_FACEBOOK.equals(key)) {
            BravePrefServiceBridge.getInstance().setThirdPartyFacebookEmbedEnabled(
                    (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_TWITTER.equals(key)) {
            BravePrefServiceBridge.getInstance().setThirdPartyTwitterEmbedEnabled(
                    (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_LINKEDIN.equals(key)) {
            BravePrefServiceBridge.getInstance().setThirdPartyLinkedinEmbedEnabled(
                    (boolean) newValue);
        } else if (PREF_CLEAR_ON_EXIT.equals(key)) {
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(PREF_CLEAR_ON_EXIT, (boolean) newValue);
            sharedPreferencesEditor.apply();
        } else if (PREF_BLOCK_TRACKERS_ADS.equals(key)) {
            BravePrefServiceBridge.getInstance().setCosmeticFilteringControlType((int) newValue);

            if ((int) newValue == 0) {
                mAdsTrakersBlockPref.setSummary(getActivity().getResources().getString(
                        R.string.block_trackers_ads_option_1));
                mAdsTrakersBlockPref.setCheckedIndex(0);
            } else if ((int) newValue == 1) {
                mAdsTrakersBlockPref.setSummary(getActivity().getResources().getString(
                        R.string.block_trackers_ads_option_2));
                mAdsTrakersBlockPref.setCheckedIndex(1);
            } else {
                mAdsTrakersBlockPref.setSummary(getActivity().getResources().getString(
                        R.string.block_trackers_ads_option_3));
                mAdsTrakersBlockPref.setCheckedIndex(2);
            }
        }

        return true;
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferences();
    }

    private void updatePreferences() {
        for (int i = 0; i < NEW_PRIVACY_PREFERENCE_ORDER.length; i++) {
            if (findPreference(NEW_PRIVACY_PREFERENCE_ORDER[i]) != null) {
                findPreference(NEW_PRIVACY_PREFERENCE_ORDER[i]).setOrder(i);
            }
        }
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES_LINK);
        String fingerprinting = BravePrefServiceBridge.getInstance().getFingerprintingControlType();
        String cosmeticFiltering =
                BravePrefServiceBridge.getInstance().getCosmeticFilteringControlType();
        String cookiesBlockType = BravePrefServiceBridge.getInstance().getCookiesBlockType();
        String getNoScriptControlType =
                BravePrefServiceBridge.getInstance().getNoScriptControlType();

        if (getNoScriptControlType.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mBlockScriptsPref.setChecked(true);
        } else {
            mBlockScriptsPref.setChecked(false);
        }

        if (cosmeticFiltering.equals(BraveShieldsContentSettings.AGGRESSIVE)) {
            mAdsTrakersBlockPref.setCheckedIndex(0);
            mAdsTrakersBlockPref.setSummary(
                    getActivity().getResources().getString(R.string.block_trackers_ads_option_1));
        } else if (cosmeticFiltering.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mAdsTrakersBlockPref.setCheckedIndex(1);
            mAdsTrakersBlockPref.setSummary(
                    getActivity().getResources().getString(R.string.block_trackers_ads_option_2));
        } else if (cosmeticFiltering.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
            mAdsTrakersBlockPref.setCheckedIndex(2);
            mAdsTrakersBlockPref.setSummary(
                    getActivity().getResources().getString(R.string.block_trackers_ads_option_3));
        }

        if (cookiesBlockType.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mBlockCrosssiteCookies.setCheckedIndex(0);
            mBlockCrosssiteCookies.setSummary(
                    getActivity().getResources().getString(R.string.block_cookies_option_1));
        } else if (cookiesBlockType.equals(BraveShieldsContentSettings.DEFAULT)) {
            mBlockCrosssiteCookies.setCheckedIndex(1);
            mBlockCrosssiteCookies.setSummary(
                    getActivity().getResources().getString(R.string.block_cross_site_cookies));
        } else if (cookiesBlockType.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
            mBlockCrosssiteCookies.setCheckedIndex(2);
            mBlockCrosssiteCookies.setSummary(
                    getActivity().getResources().getString(R.string.block_cookies_option_3));
        }

        if (fingerprinting.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mFingerprintingProtectionPref.setCheckedIndex(0);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_1));
        } else if (fingerprinting.equals(BraveShieldsContentSettings.DEFAULT)) {
            mFingerprintingProtectionPref.setCheckedIndex(1);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_2));
        } else if (fingerprinting.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
            mFingerprintingProtectionPref.setCheckedIndex(2);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_3));
        }

        mSearchSuggestions.setChecked(mPrefServiceBridge.getBoolean(Pref.SEARCH_SUGGEST_ENABLED));

        mCanMakePayment.setTitle(getActivity().getResources().getString(
                R.string.settings_can_make_payment_toggle_label));
        mCanMakePayment.setSummary("");

        mSendP3A.setTitle(
                getActivity().getResources().getString(R.string.send_p3a_analytics_title));
        mSendP3A.setSummary(
                getActivity().getResources().getString(R.string.send_p3a_analytics_summary));

        if (BraveConfig.P3A_ENABLED) {
            mSendP3A.setChecked(BravePrefServiceBridge.getInstance().getP3AEnabled());
        } else {
            getPreferenceScreen().removePreference(mSendP3A);
        }

        mWebrtcPolicy.setSummary(
                webrtcPolicyToString(BravePrefServiceBridge.getInstance().getWebrtcPolicy()));
        mAutocompleteTopSites
                .setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED));
        mAutocompleteBraveSuggestedSites.setChecked(
                UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.BRAVE_SUGGESTED_SITE_SUGGESTIONS_ENABLED));
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

    private String webrtcPolicyToString(@BraveWebrtcPolicyPreference.WebrtcPolicy int policy) {
        switch (policy) {
            case BraveWebrtcPolicyPreference.WebrtcPolicy.DEFAULT:
                return getActivity().getResources().getString(
                        R.string.settings_webrtc_policy_default);
            case BraveWebrtcPolicyPreference.WebrtcPolicy.DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES:
                return getActivity().getResources().getString(
                        R.string.settings_webrtc_policy_default_public_and_private_interfaces);
            case BraveWebrtcPolicyPreference.WebrtcPolicy.DEFAULT_PUBLIC_INTERFACE_ONLY:
                return getActivity().getResources().getString(
                        R.string.settings_webrtc_policy_default_public_interface_only);
            case BraveWebrtcPolicyPreference.WebrtcPolicy.DISABLE_NON_PROXIED_UDP:
                return getActivity().getResources().getString(
                        R.string.settings_webrtc_policy_disable_non_proxied_udp);
        }
        assert false : "Setting is out of range!";
        return "";
    }
}

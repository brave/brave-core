/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.privacy.settings.PrivacySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveWebrtcPolicyPreference;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.components.browser_ui.settings.ChromeBaseCheckBoxPreference;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

public class BravePrivacySettings extends PrivacySettings {
    private static final String PREF_HTTPSE = "httpse";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_SEND_P3A = "send_p3a_analytics";
    private static final String PREF_SYNC_AND_SERVICES_LINK = "sync_and_services_link";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_AUTOCOMPLETE_TOP_SITES = "autocomplete_top_sites";
    private static final String PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES = "autocomplete_brave_suggested_sites";
    private static final String PREF_SOCIAL_BLOCKING = "brave_shields_social_blocking";
    private static final String PREF_SOCIAL_BLOCKING_GOOGLE = "social_blocking_google";
    private static final String PREF_SOCIAL_BLOCKING_FACEBOOK = "social_blocking_facebook";
    private static final String PREF_SOCIAL_BLOCKING_TWITTER = "social_blocking_twitter";
    private static final String PREF_SOCIAL_BLOCKING_LINKEDIN = "social_blocking_linkedin";
    private static final String PREF_DO_NOT_TRACK = "do_not_track";
    private static final String PREF_WEBRTC_POLICY = "webrtc_policy";

    private final PrefService mPrefServiceBridge = UserPrefs.get(Profile.getLastUsedRegularProfile());
    private final ChromeManagedPreferenceDelegate mManagedPreferenceDelegate =
            createManagedPreferenceDelegate();
    private ChromeSwitchPreference mSearchSuggestions;
    private ChromeSwitchPreference mAutocompleteTopSites;
    private ChromeSwitchPreference mAutocompleteBraveSuggestedSites;
    private ChromeBaseCheckBoxPreference mHttpsePref;
    private ChromeBaseCheckBoxPreference mAdBlockPref;
    private ChromeBaseCheckBoxPreference mFingerprintingProtectionPref;
    private ChromeBaseCheckBoxPreference mCloseTabsOnExitPref;
    private ChromeBaseCheckBoxPreference mSendP3A;
    private PreferenceCategory mSocialBlockingCategory;
    private ChromeSwitchPreference mSocialBlockingGoogle;
    private ChromeSwitchPreference mSocialBlockingFacebook;
    private ChromeSwitchPreference mSocialBlockingTwitter;
    private ChromeSwitchPreference mSocialBlockingLinkedin;
    private ChromeBasePreference mWebrtcPolicy;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_privacy_preferences);

        mHttpsePref = (ChromeBaseCheckBoxPreference) findPreference(PREF_HTTPSE);
        mHttpsePref.setOnPreferenceChangeListener(this);

        mAdBlockPref = (ChromeBaseCheckBoxPreference) findPreference(PREF_AD_BLOCK);
        mAdBlockPref.setOnPreferenceChangeListener(this);

        mFingerprintingProtectionPref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_FINGERPRINTING_PROTECTION);
        mFingerprintingProtectionPref.setOnPreferenceChangeListener(this);

        mCloseTabsOnExitPref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_CLOSE_TABS_ON_EXIT);
        mCloseTabsOnExitPref.setOnPreferenceChangeListener(this);

        mSendP3A =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_SEND_P3A);
        mSendP3A.setOnPreferenceChangeListener(this);

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        mAutocompleteTopSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_TOP_SITES);
        mAutocompleteTopSites.setOnPreferenceChangeListener(this);

        mAutocompleteBraveSuggestedSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_BRAVE_SUGGESTED_SITES);
        mAutocompleteBraveSuggestedSites.setOnPreferenceChangeListener(this);

        mSocialBlockingCategory = (PreferenceCategory) findPreference(PREF_SOCIAL_BLOCKING);
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
        int order = findPreference(PREF_DO_NOT_TRACK).getOrder();
        mCloseTabsOnExitPref.setOrder(++order);
        if (BraveConfig.P3A_ENABLED) {
            mSendP3A.setOrder(++order);
            mSendP3A.setChecked(BravePrefServiceBridge.getInstance().getP3AEnabled());
        } else {
            getPreferenceScreen().removePreference(mSendP3A);
        }
        mHttpsePref.setOrder(++order);
        mAdBlockPref.setOrder(++order);
        mFingerprintingProtectionPref.setOrder(++order);
        mSearchSuggestions.setOrder(++order);
        mWebrtcPolicy.setOrder(++order);
        mWebrtcPolicy.setSummary(
                webrtcPolicyToString(BravePrefServiceBridge.getInstance().getWebrtcPolicy()));
        mAutocompleteTopSites
                .setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED));
        mAutocompleteTopSites.setOrder(++order);
        mAutocompleteBraveSuggestedSites.setChecked(
                UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.BRAVE_SUGGESTED_SITE_SUGGESTIONS_ENABLED));
        mAutocompleteBraveSuggestedSites.setOrder(++order);
        mSocialBlockingCategory.setOrder(++order);
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

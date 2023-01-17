/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.brave_shields.mojom.CookieListOptInPageAndroidHandler;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.metrics.UmaSessionStats;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.privacy.settings.PrivacySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.safe_browsing.settings.NoGooglePlayServicesDialog;
import org.chromium.chrome.browser.settings.BraveDialogPreference;
import org.chromium.chrome.browser.settings.BravePreferenceDialogFragment;
import org.chromium.chrome.browser.settings.BraveWebrtcPolicyPreference;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.chrome.browser.shields.CookieListOptInServiceFactory;
import org.chromium.components.browser_ui.settings.ChromeBaseCheckBoxPreference;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.gms.ChromiumPlayServicesAvailability;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class BravePrivacySettings extends PrivacySettings implements ConnectionErrorHandler {
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
    private static final String PREF_HTTPS_FIRST_MODE = "https_first_mode";
    private static final String PREF_INCOGNITO_LOCK = "incognito_lock";
    private static final String PREF_PHONE_AS_A_SECURITY_KEY = "phone_as_a_security_key";
    private static final String PREF_FINGERPRINT_LANGUAGE = "fingerprint_language";

    // brave Prefs
    private static final String PREF_BRAVE_SHIELDS_GLOBALS_SECTION =
            "brave_shields_globals_section";
    private static final String PREF_CLEAR_DATA_SECTION = "clear_data_section";
    private static final String PREF_BRAVE_SOCIAL_BLOCKING_SECTION =
            "brave_social_blocking_section";
    private static final String PREF_OTHER_PRIVACY_SETTINGS_SECTION =
            "other_privacy_settings_section";

    private static final String PREF_HTTPSE = "httpse";
    private static final String PREF_DE_AMP = "de_amp";
    private static final String PREF_DEBOUNCE = "debounce";
    private static final String PREF_IPFS_GATEWAY = "ipfs_gateway";
    private static final String PREF_BLOCK_COOKIE_CONSENT_NOTICES = "block_cookie_consent_notices";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_BLOCK_SCRIPTS = "scripts_block";
    public static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_SEND_P3A = "send_p3a_analytics";
    private static final String PREF_SEND_CRASH_REPORTS = "send_crash_reports";
    private static final String PREF_BRAVE_STATS_USAGE_PING = "brave_stats_usage_ping";
    private static final String PREF_SEARCH_SUGGESTIONS = "search_suggestions";
    private static final String PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR =
            "show_autocomplete_in_address_bar";
    private static final String PREF_AUTOCOMPLETE_TOP_SITES = "autocomplete_top_sites";
    private static final String PREF_SOCIAL_BLOCKING_GOOGLE = "social_blocking_google";
    private static final String PREF_SOCIAL_BLOCKING_FACEBOOK = "social_blocking_facebook";
    private static final String PREF_SOCIAL_BLOCKING_TWITTER = "social_blocking_twitter";
    private static final String PREF_SOCIAL_BLOCKING_LINKEDIN = "social_blocking_linkedin";
    private static final String PREF_WEBRTC_POLICY = "webrtc_policy";
    private static final String PREF_UNSTOPPABLE_DOMAINS = "unstoppable_domains";
    private static final String PREF_ENS = "ens";
    private static final String PREF_SNS = "sns";
    private static final String PREF_HTTPS_ONLY_MODE_ENABLED_SAVED_STATE =
            "https_only_mode_enabled_saved_state";

    public static final String PREF_BLOCK_TRACKERS_ADS = "block_trackers_ads";
    private static final String PREF_BLOCK_CROSS_SITE_COOKIES = "block_cross_site_cookies";
    private static final String PREF_SHIELDS_SUMMARY = "shields_summary";
    private static final String PREF_CLEAR_ON_EXIT = "clear_on_exit";

    private static final String[] NEW_PRIVACY_PREFERENCE_ORDER = {
            PREF_BRAVE_SHIELDS_GLOBALS_SECTION, //  shields globals  section
            PREF_SHIELDS_SUMMARY, PREF_BLOCK_TRACKERS_ADS, PREF_DE_AMP, PREF_DEBOUNCE, PREF_HTTPSE,
            PREF_HTTPS_FIRST_MODE, PREF_BLOCK_SCRIPTS, PREF_BLOCK_CROSS_SITE_COOKIES,
            PREF_FINGERPRINTING_PROTECTION, PREF_FINGERPRINT_LANGUAGE,
            PREF_CLEAR_DATA_SECTION, //  clear data automatically  section
            PREF_CLEAR_ON_EXIT, PREF_CLEAR_BROWSING_DATA,
            PREF_BRAVE_SOCIAL_BLOCKING_SECTION, // social blocking section
            PREF_SOCIAL_BLOCKING_GOOGLE, PREF_SOCIAL_BLOCKING_FACEBOOK,
            PREF_SOCIAL_BLOCKING_TWITTER, PREF_SOCIAL_BLOCKING_LINKEDIN,
            PREF_OTHER_PRIVACY_SETTINGS_SECTION, // other section
            PREF_WEBRTC_POLICY, PREF_SAFE_BROWSING, PREF_INCOGNITO_LOCK, PREF_CAN_MAKE_PAYMENT,
            PREF_UNSTOPPABLE_DOMAINS, PREF_ENS, PREF_SNS, PREF_IPFS_GATEWAY, PREF_SECURE_DNS,
            PREF_BLOCK_COOKIE_CONSENT_NOTICES, PREF_DO_NOT_TRACK, PREF_PHONE_AS_A_SECURITY_KEY,
            PREF_CLOSE_TABS_ON_EXIT, PREF_SEND_P3A, PREF_SEND_CRASH_REPORTS,
            PREF_BRAVE_STATS_USAGE_PING, PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR,
            PREF_SEARCH_SUGGESTIONS, PREF_AUTOCOMPLETE_TOP_SITES, PREF_USAGE_STATS,
            PREF_PRIVACY_SANDBOX};

    private final int STRICT = 0;
    private final int STANDARD = 1;
    private final int ALLOW = 2;

    private final PrefService mPrefServiceBridge = UserPrefs.get(Profile.getLastUsedRegularProfile());
    private final PrivacyPreferencesManagerImpl mPrivacyPrefManager =
            PrivacyPreferencesManagerImpl.getInstance();
    private final ChromeManagedPreferenceDelegate mManagedPreferenceDelegate =
            createManagedPreferenceDelegate();
    private ChromeSwitchPreference mSearchSuggestions;
    private ChromeSwitchPreference mCanMakePayment;
    private BraveDialogPreference mAdsTrakersBlockPref;
    private BraveDialogPreference mBlockCrosssiteCookies;
    private ChromeSwitchPreference mShowAutocompleteInAddressBar;
    private ChromeSwitchPreference mAutocompleteTopSites;
    private ChromeSwitchPreference mHttpsePref;
    private ChromeSwitchPreference mDeAmpPref;
    private ChromeSwitchPreference mDebouncePref;
    private ChromeSwitchPreference mHttpsFirstModePref;
    private BraveDialogPreference mFingerprintingProtectionPref;
    private ChromeSwitchPreference mBlockScriptsPref;
    private ChromeSwitchPreference mCloseTabsOnExitPref;
    private ChromeSwitchPreference mSendP3A;
    private ChromeSwitchPreference mSendCrashReports;
    private ChromeSwitchPreference mBraveStatsUsagePing;
    private ChromeSwitchPreference mIpfsGatewayPref;
    private ChromeSwitchPreference mBlockCookieConsentNoticesPref;
    private PreferenceCategory mSocialBlockingCategory;
    private ChromeSwitchPreference mSocialBlockingGoogle;
    private ChromeSwitchPreference mHttpsEverywhere;
    private ChromeSwitchPreference mSocialBlockingFacebook;
    private ChromeSwitchPreference mSocialBlockingTwitter;
    private ChromeSwitchPreference mSocialBlockingLinkedin;
    private ChromeBasePreference mWebrtcPolicy;
    private ChromeSwitchPreference mClearBrowsingDataOnExit;
    private Preference mUstoppableDomains;
    private ChromeSwitchPreference mFingerprntLanguagePref;
    private CookieListOptInPageAndroidHandler mCookieListOptInPageAndroidHandler;

    @Override
    public void onConnectionError(MojoException e) {
        mCookieListOptInPageAndroidHandler = null;
        initCookieListOptInPageAndroidHandler();
    }

    private void initCookieListOptInPageAndroidHandler() {
        if (mCookieListOptInPageAndroidHandler != null) {
            return;
        }

        mCookieListOptInPageAndroidHandler =
                CookieListOptInServiceFactory.getInstance().getCookieListOptInPageAndroidHandler(
                        this);
    }

    @Override
    public void onDestroy() {
        if (mCookieListOptInPageAndroidHandler != null) {
            mCookieListOptInPageAndroidHandler.close();
        }
        super.onDestroy();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        // override title
        getActivity().setTitle(R.string.brave_shields_and_privacy);

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_privacy_preferences);

        initCookieListOptInPageAndroidHandler();

        mHttpsePref = (ChromeSwitchPreference) findPreference(PREF_HTTPSE);
        mHttpsePref.setOnPreferenceChangeListener(this);

        mDeAmpPref = (ChromeSwitchPreference) findPreference(PREF_DE_AMP);
        mDeAmpPref.setOnPreferenceChangeListener(this);

        if (ChromeFeatureList.isEnabled(BraveFeatureList.DEBOUNCE)) {
            mDebouncePref = (ChromeSwitchPreference) findPreference(PREF_DEBOUNCE);
            mDebouncePref.setOnPreferenceChangeListener(this);
        } else {
            removePreferenceIfPresent(PREF_DEBOUNCE);
        }

        mHttpsFirstModePref = (ChromeSwitchPreference) findPreference(PREF_HTTPS_FIRST_MODE);
        mHttpsFirstModePref.setVisible(mHttpsePref.isChecked());

        mCanMakePayment = (ChromeSwitchPreference) findPreference(PREF_CAN_MAKE_PAYMENT);
        mCanMakePayment.setOnPreferenceChangeListener(this);

        mAdsTrakersBlockPref = (BraveDialogPreference) findPreference(PREF_BLOCK_TRACKERS_ADS);
        mAdsTrakersBlockPref.setOnPreferenceChangeListener(this);

        mUstoppableDomains = (Preference) findPreference(PREF_UNSTOPPABLE_DOMAINS);
        mUstoppableDomains.setOnPreferenceChangeListener(this);

        mIpfsGatewayPref = (ChromeSwitchPreference) findPreference(PREF_IPFS_GATEWAY);
        mIpfsGatewayPref.setOnPreferenceChangeListener(this);

        mBlockCookieConsentNoticesPref =
                (ChromeSwitchPreference) findPreference(PREF_BLOCK_COOKIE_CONSENT_NOTICES);
        mBlockCookieConsentNoticesPref.setOnPreferenceChangeListener(this);

        mBlockCrosssiteCookies =
                (BraveDialogPreference) findPreference(PREF_BLOCK_CROSS_SITE_COOKIES);
        mBlockCrosssiteCookies.setOnPreferenceChangeListener(this);

        mBlockScriptsPref = (ChromeSwitchPreference) findPreference(PREF_BLOCK_SCRIPTS);
        mBlockScriptsPref.setOnPreferenceChangeListener(this);

        mFingerprintingProtectionPref =
                (BraveDialogPreference) findPreference(PREF_FINGERPRINTING_PROTECTION);
        mFingerprintingProtectionPref.setOnPreferenceChangeListener(this);

        mFingerprntLanguagePref =
                (ChromeSwitchPreference) findPreference(PREF_FINGERPRINT_LANGUAGE);
        mFingerprntLanguagePref.setOnPreferenceChangeListener(this);

        mCloseTabsOnExitPref = (ChromeSwitchPreference) findPreference(PREF_CLOSE_TABS_ON_EXIT);
        mCloseTabsOnExitPref.setOnPreferenceChangeListener(this);

        mClearBrowsingDataOnExit = (ChromeSwitchPreference) findPreference(PREF_CLEAR_ON_EXIT);
        mClearBrowsingDataOnExit.setOnPreferenceChangeListener(this);

        mSendP3A = (ChromeSwitchPreference) findPreference(PREF_SEND_P3A);
        mSendP3A.setOnPreferenceChangeListener(this);

        mSendCrashReports = (ChromeSwitchPreference) findPreference(PREF_SEND_CRASH_REPORTS);
        mSendCrashReports.setOnPreferenceChangeListener(this);
        mBraveStatsUsagePing = (ChromeSwitchPreference) findPreference(PREF_BRAVE_STATS_USAGE_PING);
        mBraveStatsUsagePing.setOnPreferenceChangeListener(this);

        mSearchSuggestions = (ChromeSwitchPreference) findPreference(PREF_SEARCH_SUGGESTIONS);
        mSearchSuggestions.setOnPreferenceChangeListener(this);
        mSearchSuggestions.setManagedPreferenceDelegate(mManagedPreferenceDelegate);

        mShowAutocompleteInAddressBar =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR);
        mShowAutocompleteInAddressBar.setOnPreferenceChangeListener(this);

        mAutocompleteTopSites = (ChromeSwitchPreference) findPreference(PREF_AUTOCOMPLETE_TOP_SITES);
        mAutocompleteTopSites.setOnPreferenceChangeListener(this);

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

        removePreferenceIfPresent(PREF_AD_BLOCK);
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES_LINK);
        removePreferenceIfPresent(PREF_NETWORK_PREDICTIONS);
        removePreferenceIfPresent(PREF_PRIVACY_SANDBOX);
        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_SAFE_BROWSING)) {
            removePreferenceIfPresent(PREF_SAFE_BROWSING);
        } else {
            Preference preference = getPreferenceScreen().findPreference(PREF_SAFE_BROWSING);
            if (preference != null) {
                preference.setOnPreferenceClickListener((pref) -> {
                    if (!ChromiumPlayServicesAvailability.isGooglePlayServicesAvailable(
                                getActivity())) {
                        NoGooglePlayServicesDialog.create(getContext()).show();
                        // Don't show the menu if Google Play Services are not available
                        return true;
                    }

                    return false;
                });
            }
        }

        if (mCookieListOptInPageAndroidHandler != null) {
            mCookieListOptInPageAndroidHandler.shouldShowDialog(shouldShowDialog -> {
                if (!shouldShowDialog) {
                    removePreferenceIfPresent(PREF_BLOCK_COOKIE_CONSENT_NOTICES);
                }
            });
        }

        updateBravePreferences();
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
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        if (PREF_HTTPSE.equals(key)) {
            boolean newValueBool = (boolean) newValue;
            BraveShieldsContentSettings.setHTTPSEverywherePref(newValueBool);
            mHttpsFirstModePref.setVisible(newValueBool);
            if (newValueBool) {
                // Restore state of HTTPS_ONLY_MODE.
                UserPrefs.get(Profile.getLastUsedRegularProfile())
                        .setBoolean(Pref.HTTPS_ONLY_MODE_ENABLED,
                                ContextUtils.getAppSharedPreferences().getBoolean(
                                        PREF_HTTPS_ONLY_MODE_ENABLED_SAVED_STATE, false));
                mHttpsFirstModePref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                                       .getBoolean(Pref.HTTPS_ONLY_MODE_ENABLED));
            } else {
                // Save state for HTTPS_ONLY_MODE and disable it.
                sharedPreferencesEditor.putBoolean(PREF_HTTPS_ONLY_MODE_ENABLED_SAVED_STATE,
                        UserPrefs.get(Profile.getLastUsedRegularProfile())
                                .getBoolean(Pref.HTTPS_ONLY_MODE_ENABLED));
                UserPrefs.get(Profile.getLastUsedRegularProfile())
                        .setBoolean(Pref.HTTPS_ONLY_MODE_ENABLED, newValueBool);
            }
        } else if (PREF_HTTPS_FIRST_MODE.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(Pref.HTTPS_ONLY_MODE_ENABLED, (boolean) newValue);
        } else if (PREF_DE_AMP.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.DE_AMP_PREF_ENABLED, (boolean) newValue);
        } else if (PREF_DEBOUNCE.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.DEBOUNCE_ENABLED, (boolean) newValue);
        } else if (PREF_IPFS_GATEWAY.equals(key)) {
            BravePrivacySettingsIPFSUtils.setIPFSGatewayPref((boolean) newValue);
        } else if (PREF_BLOCK_COOKIE_CONSENT_NOTICES.equals(key)) {
            if (mCookieListOptInPageAndroidHandler != null) {
                mCookieListOptInPageAndroidHandler.enableFilter((boolean) newValue);
            }
        } else if (PREF_FINGERPRINTING_PROTECTION.equals(key)) {
            if (newValue instanceof String) {
                final String newStringValue = String.valueOf(newValue);
                switch (newStringValue) {
                    case BraveShieldsContentSettings.BLOCK_RESOURCE:
                        BraveShieldsContentSettings.setFingerprintingPref(newStringValue);
                        mFingerprintingProtectionPref.setSummary(
                                getActivity().getResources().getString(
                                        R.string.block_fingerprinting_option_1));
                        mFingerprintingProtectionPref.setCheckedIndex(0);
                        break;
                    case BraveShieldsContentSettings.DEFAULT:
                        BraveShieldsContentSettings.setFingerprintingPref(newStringValue);
                        mFingerprintingProtectionPref.setSummary(
                                getActivity().getResources().getString(
                                        R.string.block_fingerprinting_option_2));
                        mFingerprintingProtectionPref.setCheckedIndex(1);
                        break;
                    case BraveShieldsContentSettings.ALLOW_RESOURCE:
                    default:
                        BraveShieldsContentSettings.setFingerprintingPref(
                                BraveShieldsContentSettings.ALLOW_RESOURCE);
                        mFingerprintingProtectionPref.setSummary(
                                getActivity().getResources().getString(
                                        R.string.block_fingerprinting_option_3));
                        mFingerprintingProtectionPref.setCheckedIndex(2);
                        break;
                }
            }
        } else if (PREF_FINGERPRINT_LANGUAGE.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.REDUCE_LANGUAGE_ENABLED, (boolean) newValue);
        } else if (PREF_BLOCK_CROSS_SITE_COOKIES.equals(key)) {
            switch ((int) newValue) {
                case STRICT:
                    BraveShieldsContentSettings.setCookiesPref(
                            BraveShieldsContentSettings.BLOCK_RESOURCE);
                    mBlockCrosssiteCookies.setSummary(getActivity().getResources().getString(
                            R.string.block_cookies_option_1));
                    mBlockCrosssiteCookies.setCheckedIndex(0);
                    break;
                case STANDARD:
                    BraveShieldsContentSettings.setCookiesPref(
                            BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE);
                    mBlockCrosssiteCookies.setSummary(getActivity().getResources().getString(
                            R.string.block_cookies_option_2));
                    mBlockCrosssiteCookies.setCheckedIndex(1);
                // fall through
                case ALLOW:
                default:
                    BraveShieldsContentSettings.setCookiesPref(
                            BraveShieldsContentSettings.ALLOW_RESOURCE);
                    mBlockCrosssiteCookies.setSummary(getActivity().getResources().getString(
                            R.string.block_cookies_option_3));
                    mBlockCrosssiteCookies.setCheckedIndex(2);
            }
        } else if (PREF_BLOCK_SCRIPTS.equals(key)) {
            BraveShieldsContentSettings.setJavascriptPref((boolean) newValue);
        } else if (PREF_CLOSE_TABS_ON_EXIT.equals(key)) {
            sharedPreferencesEditor.putBoolean(PREF_CLOSE_TABS_ON_EXIT, (boolean) newValue);
        } else if (PREF_SEND_P3A.equals(key)) {
            BraveLocalState.get().setBoolean(BravePref.P3A_ENABLED, (boolean) newValue);
            BraveLocalState.commitPendingWrite();
        } else if (PREF_SEND_CRASH_REPORTS.equals(key)) {
            UmaSessionStats.changeMetricsReportingConsent((boolean) newValue);
        } else if (PREF_BRAVE_STATS_USAGE_PING.equals(key)) {
            BraveLocalState.get().setBoolean(BravePref.STATS_REPORTING_ENABLED, (boolean) newValue);
        } else if (PREF_SEARCH_SUGGESTIONS.equals(key)) {
            mPrefServiceBridge.setBoolean(Pref.SEARCH_SUGGEST_ENABLED, (boolean) newValue);
        } else if (PREF_SHOW_AUTOCOMPLETE_IN_ADDRESS_BAR.equals(key)) {
            boolean autocompleteEnabled = (boolean) newValue;
            mSearchSuggestions.setEnabled(autocompleteEnabled);
            mAutocompleteTopSites.setEnabled(autocompleteEnabled);
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.AUTOCOMPLETE_ENABLED, autocompleteEnabled);
        } else if (PREF_AUTOCOMPLETE_TOP_SITES.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_GOOGLE.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.GOOGLE_LOGIN_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_FACEBOOK.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.FB_EMBED_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_TWITTER.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.TWITTER_EMBED_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_LINKEDIN.equals(key)) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.LINKED_IN_EMBED_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_CLEAR_ON_EXIT.equals(key)) {
            sharedPreferencesEditor.putBoolean(PREF_CLEAR_ON_EXIT, (boolean) newValue);
        } else if (PREF_BLOCK_TRACKERS_ADS.equals(key)) {
            if (newValue instanceof String) {
                final String newStringValue = String.valueOf(newValue);
                switch (newStringValue) {
                    case BraveShieldsContentSettings.BLOCK_RESOURCE:
                        BraveShieldsContentSettings.setTrackersPref(newStringValue);
                        mAdsTrakersBlockPref.setSummary(getActivity().getResources().getString(
                                R.string.block_trackers_ads_option_1));
                        mAdsTrakersBlockPref.setCheckedIndex(0);
                        break;
                    case BraveShieldsContentSettings.DEFAULT:
                        BraveShieldsContentSettings.setTrackersPref(newStringValue);
                        mAdsTrakersBlockPref.setSummary(getActivity().getResources().getString(
                                R.string.block_trackers_ads_option_2));
                        mAdsTrakersBlockPref.setCheckedIndex(1);
                        break;
                    default:
                        BraveShieldsContentSettings.setTrackersPref(
                                BraveShieldsContentSettings.ALLOW_RESOURCE);
                        mAdsTrakersBlockPref.setSummary(getActivity().getResources().getString(
                                R.string.block_trackers_ads_option_3));
                        mAdsTrakersBlockPref.setCheckedIndex(2);
                        break;
                }
            }
        }

        sharedPreferencesEditor.apply();

        return true;
    }

    private void updateBravePreferences() {
        for (int i = 0; i < NEW_PRIVACY_PREFERENCE_ORDER.length; i++) {
            if (findPreference(NEW_PRIVACY_PREFERENCE_ORDER[i]) != null) {
                findPreference(NEW_PRIVACY_PREFERENCE_ORDER[i]).setOrder(i);
            }
        }
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES_LINK);

        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();

        String blockAdTrackersPref = BraveShieldsContentSettings.getTrackersPref();
        int cookiesBlockPref = sharedPreferences.getInt(PREF_BLOCK_CROSS_SITE_COOKIES, 1);
        String fingerprintingPref = BraveShieldsContentSettings.getFingerprintingPref();

        mBlockScriptsPref.setChecked(BraveShieldsContentSettings.getJavascriptPref());

        // HTTPSE and HTTPS only mode
        final boolean httpseEnabled = BraveShieldsContentSettings.getHTTPSEverywherePref();
        mHttpsePref.setChecked(httpseEnabled);
        if (httpseEnabled) {
            mHttpsFirstModePref.setVisible(httpseEnabled);
            mHttpsFirstModePref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                                   .getBoolean(Pref.HTTPS_ONLY_MODE_ENABLED));
        } else {
            mHttpsFirstModePref.setChecked(httpseEnabled);
        }

        // IPFS Gateway
        mIpfsGatewayPref.setChecked(BravePrivacySettingsIPFSUtils.getIPFSGatewayPref());

        if (blockAdTrackersPref.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mAdsTrakersBlockPref.setCheckedIndex(0);
            mAdsTrakersBlockPref.setSummary(
                    getActivity().getResources().getString(R.string.block_trackers_ads_option_1));
        } else if (blockAdTrackersPref.equals(BraveShieldsContentSettings.DEFAULT)) {
            mAdsTrakersBlockPref.setCheckedIndex(1);
            mAdsTrakersBlockPref.setSummary(
                    getActivity().getResources().getString(R.string.block_trackers_ads_option_2));
        } else if (blockAdTrackersPref.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
            mAdsTrakersBlockPref.setCheckedIndex(2);
            mAdsTrakersBlockPref.setSummary(
                    getActivity().getResources().getString(R.string.block_trackers_ads_option_3));
        }

        if (cookiesBlockPref == STRICT) {
            mBlockCrosssiteCookies.setCheckedIndex(0);
            mBlockCrosssiteCookies.setSummary(
                    getActivity().getResources().getString(R.string.block_cookies_option_1));
        } else if (cookiesBlockPref == STANDARD) {
            mBlockCrosssiteCookies.setCheckedIndex(1);
            mBlockCrosssiteCookies.setSummary(
                    getActivity().getResources().getString(R.string.block_cookies_option_2));
        } else if (cookiesBlockPref == ALLOW) {
            mBlockCrosssiteCookies.setCheckedIndex(2);
            mBlockCrosssiteCookies.setSummary(
                    getActivity().getResources().getString(R.string.block_cookies_option_3));
        }

        if (fingerprintingPref.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mFingerprintingProtectionPref.setCheckedIndex(0);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_1));
        } else if (fingerprintingPref.equals(BraveShieldsContentSettings.DEFAULT)) {
            mFingerprintingProtectionPref.setCheckedIndex(1);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_2));
        } else if (fingerprintingPref.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
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
            mSendP3A.setChecked(BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED));
        } else {
            getPreferenceScreen().removePreference(mSendP3A);
        }

        mSendCrashReports.setChecked(mPrivacyPrefManager.isUsageAndCrashReportingPermittedByUser());

        mBraveStatsUsagePing.setChecked(
                BraveLocalState.get().getBoolean(BravePref.STATS_REPORTING_ENABLED));

        mWebrtcPolicy.setSummary(
                webrtcPolicyToString(BravePrefServiceBridge.getInstance().getWebrtcPolicy()));

        mAutocompleteTopSites.setChecked(
                UserPrefs.get(Profile.getLastUsedRegularProfile())
                        .getBoolean(BravePref.TOP_SITE_SUGGESTIONS_ENABLED));

        mClearBrowsingDataOnExit.setChecked(
                sharedPreferences.getBoolean(PREF_CLEAR_ON_EXIT, false));

        boolean autocompleteEnabled = UserPrefs.get(Profile.getLastUsedRegularProfile())
                                              .getBoolean(BravePref.AUTOCOMPLETE_ENABLED);
        mShowAutocompleteInAddressBar.setChecked(autocompleteEnabled);
        mSearchSuggestions.setEnabled(autocompleteEnabled);
        mAutocompleteTopSites.setEnabled(autocompleteEnabled);
        mFingerprntLanguagePref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                                   .getBoolean(BravePref.REDUCE_LANGUAGE_ENABLED));
        if (mCookieListOptInPageAndroidHandler != null) {
            mCookieListOptInPageAndroidHandler.isFilterListEnabled(
                    isEnabled -> { mBlockCookieConsentNoticesPref.setChecked(isEnabled); });
        }
        // Debounce
        if (mDebouncePref != null) {
            mDebouncePref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                             .getBoolean(BravePref.DEBOUNCE_ENABLED));
        }

        // Social blocking
        if (mSocialBlockingGoogle != null) {
            mSocialBlockingGoogle.setChecked(
                    UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(BravePref.GOOGLE_LOGIN_CONTROL_TYPE));
        }
        if (mSocialBlockingFacebook != null) {
            mSocialBlockingFacebook.setChecked(
                    UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(BravePref.FB_EMBED_CONTROL_TYPE));
        }
        if (mSocialBlockingTwitter != null) {
            mSocialBlockingTwitter.setChecked(
                    UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(BravePref.TWITTER_EMBED_CONTROL_TYPE));
        }
        if (mSocialBlockingLinkedin != null) {
            mSocialBlockingLinkedin.setChecked(
                    UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(BravePref.LINKED_IN_EMBED_CONTROL_TYPE));
        }

        if (mDeAmpPref != null) {
            mDeAmpPref.setChecked(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                          .getBoolean(BravePref.DE_AMP_PREF_ENABLED));
        }

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_WALLET_SNS)) {
            removePreferenceIfPresent(PREF_SNS);
        }
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

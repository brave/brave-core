/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.brave_shields.mojom.FilterListAndroidHandler;
import org.chromium.brave_shields.mojom.FilterListConstants;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.metrics.ChangeMetricsReportingStateCalledFrom;
import org.chromium.chrome.browser.metrics.UmaSessionStats;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.safe_browsing.settings.NoGooglePlayServicesDialog;
import org.chromium.chrome.browser.settings.BraveDialogPreference;
import org.chromium.chrome.browser.settings.BravePreferenceDialogFragment;
import org.chromium.chrome.browser.settings.BraveWebrtcPolicyPreference;
import org.chromium.chrome.browser.shields.FilterListServiceFactory;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.gms.ChromiumPlayServicesAvailability;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

/**
 * Fragment to keep track of the all the brave privacy related preferences.
 */
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
    private static final String PREF_PRIVACY_SECTION = "privacy_section";
    private static final String PREF_THIRD_PARTY_COOKIES = "third_party_cookies";
    private static final String PREF_SECURITY_SECTION = "security_section";
    private static final String PREF_PRIVACY_GUIDE = "privacy_guide";

    // brave Prefs
    private static final String PREF_BRAVE_SHIELDS_GLOBALS_SECTION =
            "brave_shields_globals_section";
    private static final String PREF_CLEAR_DATA_SECTION = "clear_data_section";
    private static final String PREF_BRAVE_SOCIAL_BLOCKING_SECTION =
            "brave_social_blocking_section";
    private static final String PREF_OTHER_PRIVACY_SETTINGS_SECTION =
            "other_privacy_settings_section";

    private static final String PREF_DE_AMP = "de_amp";
    private static final String PREF_DEBOUNCE = "debounce";
    private static final String PREF_BLOCK_COOKIE_CONSENT_NOTICES = "block_cookie_consent_notices";
    private static final String PREF_BLOCK_SWITCH_TO_APP_NOTICES = "block_switch_to_app_notices";
    private static final String PREF_AD_BLOCK = "ad_block";
    private static final String PREF_BLOCK_SCRIPTS = "scripts_block";
    public static final String PREF_FINGERPRINTING_PROTECTION = "fingerprinting_protection";
    public static final String PREF_FINGERPRINTING_PROTECTION2 = "fingerprinting_protection2";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_SEND_P3A = "send_p3a_analytics";
    private static final String PREF_SEND_CRASH_REPORTS = "send_crash_reports";
    private static final String PREF_BRAVE_STATS_USAGE_PING = "brave_stats_usage_ping";
    public static final String PREF_APP_LINKS = "app_links";
    public static final String PREF_APP_LINKS_RESET = "app_links_reset";

    private static final String PREF_SOCIAL_BLOCKING_GOOGLE = "social_blocking_google";
    private static final String PREF_SOCIAL_BLOCKING_FACEBOOK = "social_blocking_facebook";
    private static final String PREF_SOCIAL_BLOCKING_TWITTER = "social_blocking_twitter";
    private static final String PREF_SOCIAL_BLOCKING_LINKEDIN = "social_blocking_linkedin";
    private static final String PREF_WEBRTC_POLICY = "webrtc_policy";
    private static final String PREF_UNSTOPPABLE_DOMAINS = "unstoppable_domains";
    private static final String PREF_CONTENT_FILTERING = "content_filtering";
    private static final String PREF_ENS = "ens";
    private static final String PREF_SNS = "sns";
    private static final String PREF_REQUEST_OTR = "request_otr";

    public static final String PREF_BLOCK_TRACKERS_ADS = "block_trackers_ads";
    private static final String PREF_BLOCK_CROSS_SITE_COOKIES = "block_cross_site_cookies";
    private static final String PREF_SHIELDS_SUMMARY = "shields_summary";
    private static final String PREF_CLEAR_ON_EXIT = "clear_on_exit";
    private static final String PREF_HTTPS_UPGRADE = "https_upgrade";
    private static final String PREF_FORGET_FIRST_PARTY_STORAGE = "forget_first_party_storage";

    private static final String[] NEW_PRIVACY_PREFERENCE_ORDER = {
        PREF_BRAVE_SHIELDS_GLOBALS_SECTION, //  shields globals  section
        PREF_SHIELDS_SUMMARY,
        PREF_BLOCK_TRACKERS_ADS,
        PREF_DE_AMP,
        PREF_DEBOUNCE,
        PREF_HTTPS_UPGRADE,
        PREF_HTTPS_FIRST_MODE,
        PREF_BLOCK_SCRIPTS,
        PREF_BLOCK_CROSS_SITE_COOKIES,
        PREF_FINGERPRINTING_PROTECTION,
        PREF_FINGERPRINTING_PROTECTION2,
        PREF_FINGERPRINT_LANGUAGE,
        PREF_CONTENT_FILTERING,
        PREF_FORGET_FIRST_PARTY_STORAGE,
        PREF_CLEAR_DATA_SECTION, //  clear data automatically  section
        PREF_CLEAR_ON_EXIT,
        PREF_CLEAR_BROWSING_DATA,
        PREF_BRAVE_SOCIAL_BLOCKING_SECTION, // social blocking section
        PREF_SOCIAL_BLOCKING_GOOGLE,
        PREF_SOCIAL_BLOCKING_FACEBOOK,
        PREF_SOCIAL_BLOCKING_TWITTER,
        PREF_SOCIAL_BLOCKING_LINKEDIN,
        PREF_OTHER_PRIVACY_SETTINGS_SECTION, // other section
        PREF_APP_LINKS,
        PREF_WEBRTC_POLICY,
        PREF_SAFE_BROWSING,
        PREF_INCOGNITO_LOCK,
        PREF_CAN_MAKE_PAYMENT,
        PREF_UNSTOPPABLE_DOMAINS,
        PREF_ENS,
        PREF_SNS,
        PREF_REQUEST_OTR,
        PREF_SECURE_DNS,
        PREF_BLOCK_COOKIE_CONSENT_NOTICES,
        PREF_BLOCK_SWITCH_TO_APP_NOTICES,
        PREF_DO_NOT_TRACK,
        PREF_PHONE_AS_A_SECURITY_KEY,
        PREF_CLOSE_TABS_ON_EXIT,
        PREF_SEND_P3A,
        PREF_SEND_CRASH_REPORTS,
        PREF_BRAVE_STATS_USAGE_PING,
        PREF_USAGE_STATS,
        PREF_PRIVACY_SANDBOX
    };

    private static final int STRICT = 0;
    private static final int STANDARD = 1;
    private static final int ALLOW = 2;

    private final PrivacyPreferencesManagerImpl mPrivacyPrefManager =
            PrivacyPreferencesManagerImpl.getInstance();
    private ChromeSwitchPreference mCanMakePayment;
    private BraveDialogPreference mAdsTrakersBlockPref;
    private BraveDialogPreference mBlockCrosssiteCookies;
    private ChromeSwitchPreference mDeAmpPref;
    private ChromeSwitchPreference mDebouncePref;
    private ChromeSwitchPreference mHttpsFirstModePref;
    private BraveDialogPreference mHttpsUpgradePref;
    private BraveDialogPreference mFingerprintingProtectionPref;
    private ChromeSwitchPreference mFingerprintingProtection2Pref;
    private BraveDialogPreference mRequestOtrPref;
    private ChromeSwitchPreference mBlockScriptsPref;
    private ChromeSwitchPreference mForgetFirstPartyStoragePref;
    private ChromeSwitchPreference mCloseTabsOnExitPref;
    private ChromeSwitchPreference mSendP3A;
    private ChromeSwitchPreference mSendCrashReports;
    private ChromeSwitchPreference mBraveStatsUsagePing;
    private ChromeSwitchPreference mBlockCookieConsentNoticesPref;
    private ChromeSwitchPreference mBlockSwitchToAppNoticesPref;
    private PreferenceCategory mSocialBlockingCategory;
    private ChromeSwitchPreference mSocialBlockingGoogle;
    private ChromeSwitchPreference mSocialBlockingFacebook;
    private ChromeSwitchPreference mSocialBlockingTwitter;
    private ChromeSwitchPreference mSocialBlockingLinkedin;
    private ChromeSwitchPreference mAppLinks;
    private ChromeBasePreference mWebrtcPolicy;
    private ChromeSwitchPreference mClearBrowsingDataOnExit;
    private Preference mUstoppableDomains;
    private ChromeSwitchPreference mFingerprntLanguagePref;
    private FilterListAndroidHandler mFilterListAndroidHandler;

    @Override
    public void onConnectionError(MojoException e) {
        mFilterListAndroidHandler = null;
        initFilterListAndroidHandler();
    }

    private void initFilterListAndroidHandler() {
        if (mFilterListAndroidHandler != null) {
            return;
        }

        mFilterListAndroidHandler =
                FilterListServiceFactory.getInstance().getFilterListAndroidHandler(this);
    }

    @Override
    public void onDestroy() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.close();
        }
        super.onDestroy();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        // override title
        getActivity().setTitle(R.string.brave_shields_and_privacy);

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_privacy_preferences);

        initFilterListAndroidHandler();

        mDeAmpPref = (ChromeSwitchPreference) findPreference(PREF_DE_AMP);
        mDeAmpPref.setOnPreferenceChangeListener(this);

        if (ChromeFeatureList.isEnabled(BraveFeatureList.DEBOUNCE)) {
            mDebouncePref = (ChromeSwitchPreference) findPreference(PREF_DEBOUNCE);
            mDebouncePref.setOnPreferenceChangeListener(this);
        } else {
            removePreferenceIfPresent(PREF_DEBOUNCE);
        }

        mHttpsFirstModePref = (ChromeSwitchPreference) findPreference(PREF_HTTPS_FIRST_MODE);

        mHttpsUpgradePref = (BraveDialogPreference) findPreference(PREF_HTTPS_UPGRADE);
        mHttpsUpgradePref.setOnPreferenceChangeListener(this);

        boolean httpsByDefaultIsEnabled =
                ChromeFeatureList.isEnabled(BraveFeatureList.HTTPS_BY_DEFAULT);
        mHttpsFirstModePref.setVisible(!httpsByDefaultIsEnabled);
        mHttpsUpgradePref.setVisible(httpsByDefaultIsEnabled);

        mCanMakePayment = (ChromeSwitchPreference) findPreference(PREF_CAN_MAKE_PAYMENT);
        mCanMakePayment.setOnPreferenceChangeListener(this);

        mAdsTrakersBlockPref = (BraveDialogPreference) findPreference(PREF_BLOCK_TRACKERS_ADS);
        mAdsTrakersBlockPref.setOnPreferenceChangeListener(this);

        mUstoppableDomains = (Preference) findPreference(PREF_UNSTOPPABLE_DOMAINS);
        mUstoppableDomains.setOnPreferenceChangeListener(this);

        mBlockCookieConsentNoticesPref =
                (ChromeSwitchPreference) findPreference(PREF_BLOCK_COOKIE_CONSENT_NOTICES);
        mBlockCookieConsentNoticesPref.setOnPreferenceChangeListener(this);

        mBlockSwitchToAppNoticesPref =
                (ChromeSwitchPreference) findPreference(PREF_BLOCK_SWITCH_TO_APP_NOTICES);
        mBlockSwitchToAppNoticesPref.setOnPreferenceChangeListener(this);

        mBlockCrosssiteCookies =
                (BraveDialogPreference) findPreference(PREF_BLOCK_CROSS_SITE_COOKIES);
        mBlockCrosssiteCookies.setOnPreferenceChangeListener(this);

        mBlockScriptsPref = (ChromeSwitchPreference) findPreference(PREF_BLOCK_SCRIPTS);
        mBlockScriptsPref.setOnPreferenceChangeListener(this);

        mFingerprintingProtectionPref =
                (BraveDialogPreference) findPreference(PREF_FINGERPRINTING_PROTECTION);
        mFingerprintingProtectionPref.setOnPreferenceChangeListener(this);

        mFingerprintingProtection2Pref =
                (ChromeSwitchPreference) findPreference(PREF_FINGERPRINTING_PROTECTION2);
        mFingerprintingProtection2Pref.setOnPreferenceChangeListener(this);

        boolean showStrictFingerprintingMode =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SHOW_STRICT_FINGERPRINTING_MODE);

        mFingerprintingProtectionPref.setVisible(showStrictFingerprintingMode);
        mFingerprintingProtection2Pref.setVisible(!showStrictFingerprintingMode);

        mRequestOtrPref = (BraveDialogPreference) findPreference(PREF_REQUEST_OTR);
        mRequestOtrPref.setOnPreferenceChangeListener(this);

        mFingerprntLanguagePref =
                (ChromeSwitchPreference) findPreference(PREF_FINGERPRINT_LANGUAGE);
        mFingerprntLanguagePref.setOnPreferenceChangeListener(this);

        mForgetFirstPartyStoragePref =
                (ChromeSwitchPreference) findPreference(PREF_FORGET_FIRST_PARTY_STORAGE);
        mForgetFirstPartyStoragePref.setOnPreferenceChangeListener(this);
        boolean forgetFirstPartyStorageIsEnabled =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_FORGET_FIRST_PARTY_STORAGE);
        mForgetFirstPartyStoragePref.setVisible(forgetFirstPartyStorageIsEnabled);

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

        mSocialBlockingCategory =
                (PreferenceCategory) findPreference(PREF_BRAVE_SOCIAL_BLOCKING_SECTION);
        mSocialBlockingCategory.setOnPreferenceChangeListener(this);

        mSocialBlockingGoogle =
                (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_GOOGLE);

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_GOOGLE_SIGN_IN_PERMISSION)) {
            mSocialBlockingGoogle.setOnPreferenceChangeListener(this);
        } else {
            // Have to remove this preference from the right category
            if (mSocialBlockingGoogle != null) {
                mSocialBlockingCategory.removePreference(mSocialBlockingGoogle);
            }
        }

        mSocialBlockingFacebook =
                (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_FACEBOOK);
        mSocialBlockingFacebook.setOnPreferenceChangeListener(this);

        mSocialBlockingTwitter =
                (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_TWITTER);
        mSocialBlockingTwitter.setOnPreferenceChangeListener(this);

        mSocialBlockingLinkedin =
                (ChromeSwitchPreference) findPreference(PREF_SOCIAL_BLOCKING_LINKEDIN);
        mSocialBlockingLinkedin.setOnPreferenceChangeListener(this);

        mAppLinks = (ChromeSwitchPreference) findPreference(PREF_APP_LINKS);
        mAppLinks.setOnPreferenceChangeListener(this);

        boolean isAppLinksAllowed =
                ChromeSharedPreferences.getInstance().readBoolean(PREF_APP_LINKS, true);
        mAppLinks.setChecked(isAppLinksAllowed);

        mWebrtcPolicy = (ChromeBasePreference) findPreference(PREF_WEBRTC_POLICY);

        removePreferenceIfPresent(PREF_AD_BLOCK);
        removePreferenceIfPresent(PREF_SYNC_AND_SERVICES_LINK);
        removePreferenceIfPresent(PREF_NETWORK_PREDICTIONS);
        removePreferenceIfPresent(PREF_PRIVACY_SANDBOX);
        removePreferenceIfPresent(PREF_PRIVACY_SECTION);
        removePreferenceIfPresent(PREF_THIRD_PARTY_COOKIES);
        removePreferenceIfPresent(PREF_SECURITY_SECTION);
        removePreferenceIfPresent(PREF_PRIVACY_GUIDE);

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

        updateBravePreferences();
    }

    // used for displaying BraveDialogPreference
    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        if (preference instanceof BraveDialogPreference) {
            BravePreferenceDialogFragment dialogFragment =
                    BravePreferenceDialogFragment.newInstance(preference);
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
        if (PREF_HTTPS_FIRST_MODE.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(Pref.HTTPS_ONLY_MODE_ENABLED, (boolean) newValue);
        } else if (PREF_HTTPS_UPGRADE.equals(key)) {
            switch ((int) newValue) {
                case STRICT:
                    BraveShieldsContentSettings.setHttpsUpgradePref(
                            BraveShieldsContentSettings.BLOCK_RESOURCE);
                    mHttpsUpgradePref.setSummary(getActivity().getResources().getString(
                            R.string.https_upgrade_option_1));
                    mHttpsUpgradePref.setCheckedIndex(0);
                    break;
                case STANDARD:
                    BraveShieldsContentSettings.setHttpsUpgradePref(
                            BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE);
                    mHttpsUpgradePref.setSummary(getActivity().getResources().getString(
                            R.string.https_upgrade_option_2));
                    mHttpsUpgradePref.setCheckedIndex(1);
                    break;
                case ALLOW:
                default:
                    BraveShieldsContentSettings.setHttpsUpgradePref(
                            BraveShieldsContentSettings.ALLOW_RESOURCE);
                    mHttpsUpgradePref.setSummary(getActivity().getResources().getString(
                            R.string.https_upgrade_option_3));
                    mHttpsUpgradePref.setCheckedIndex(2);
                    break;
            }
        } else if (PREF_DE_AMP.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.DE_AMP_PREF_ENABLED, (boolean) newValue);
        } else if (PREF_DEBOUNCE.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.DEBOUNCE_ENABLED, (boolean) newValue);
        } else if (PREF_BLOCK_COOKIE_CONSENT_NOTICES.equals(key)) {
            if (mFilterListAndroidHandler != null) {
                mFilterListAndroidHandler.enableFilter(
                        FilterListConstants.COOKIE_LIST_UUID, (boolean) newValue);
            }
        } else if (PREF_BLOCK_SWITCH_TO_APP_NOTICES.equals(key)) {
            if (mFilterListAndroidHandler != null) {
                mFilterListAndroidHandler.enableFilter(
                        FilterListConstants.SWITCH_TO_APP_UUID, (boolean) newValue);
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
        } else if (PREF_FINGERPRINTING_PROTECTION2.equals(key)) {
            boolean protect = (boolean) newValue;
            BraveShieldsContentSettings.setFingerprintingPref(
                    protect
                            ? BraveShieldsContentSettings.DEFAULT
                            : BraveShieldsContentSettings.ALLOW_RESOURCE);
        } else if (PREF_REQUEST_OTR.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setInteger(BravePref.REQUEST_OTR_ACTION_OPTION, (int) newValue);
            updateRequestOtrPref();
        } else if (PREF_FINGERPRINT_LANGUAGE.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
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
                    break;
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
        } else if (PREF_FORGET_FIRST_PARTY_STORAGE.equals(key)) {
            BraveShieldsContentSettings.setForgetFirstPartyStoragePref((boolean) newValue);
        } else if (PREF_CLOSE_TABS_ON_EXIT.equals(key)) {
            sharedPreferencesEditor.putBoolean(
                    BravePreferenceKeys.BRAVE_CLOSE_TABS_ON_EXIT, (boolean) newValue);
        } else if (PREF_SEND_P3A.equals(key)) {
            BraveLocalState.get().setBoolean(BravePref.P3A_ENABLED, (boolean) newValue);
            BraveLocalState.commitPendingWrite();
        } else if (PREF_SEND_CRASH_REPORTS.equals(key)) {
            UmaSessionStats.changeMetricsReportingConsent(
                    (boolean) newValue, ChangeMetricsReportingStateCalledFrom.UI_SETTINGS);
        } else if (PREF_BRAVE_STATS_USAGE_PING.equals(key)) {
            BraveLocalState.get().setBoolean(BravePref.STATS_REPORTING_ENABLED, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_GOOGLE.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.GOOGLE_LOGIN_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_FACEBOOK.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.FB_EMBED_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_TWITTER.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.TWITTER_EMBED_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_SOCIAL_BLOCKING_LINKEDIN.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.LINKED_IN_EMBED_CONTROL_TYPE, (boolean) newValue);
        } else if (PREF_CLEAR_ON_EXIT.equals(key)) {
            sharedPreferencesEditor.putBoolean(
                    BravePreferenceKeys.BRAVE_CLEAR_ON_EXIT, (boolean) newValue);
        } else if (PREF_APP_LINKS.equals(key)) {
            sharedPreferencesEditor.putBoolean(PREF_APP_LINKS, (boolean) newValue);
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePrivacySettings.PREF_APP_LINKS_RESET, false);
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
        String httpsUpgradePref = BraveShieldsContentSettings.getHttpsUpgradePref();

        mBlockScriptsPref.setChecked(BraveShieldsContentSettings.getJavascriptPref());

        // HTTPS only mode
        boolean httpsByDefaultIsEnabled =
                ChromeFeatureList.isEnabled(BraveFeatureList.HTTPS_BY_DEFAULT);
        mHttpsFirstModePref.setVisible(!httpsByDefaultIsEnabled);
        mHttpsFirstModePref.setChecked(
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getBoolean(Pref.HTTPS_ONLY_MODE_ENABLED));

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
            mFingerprintingProtection2Pref.setChecked(true);
        } else if (fingerprintingPref.equals(BraveShieldsContentSettings.DEFAULT)) {
            mFingerprintingProtectionPref.setCheckedIndex(1);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_2));
            mFingerprintingProtection2Pref.setChecked(true);
        } else if (fingerprintingPref.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
            mFingerprintingProtectionPref.setCheckedIndex(2);
            mFingerprintingProtectionPref.setSummary(
                    getActivity().getResources().getString(R.string.block_fingerprinting_option_3));
            mFingerprintingProtection2Pref.setChecked(false);
        }

        if (httpsUpgradePref.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
            mHttpsUpgradePref.setCheckedIndex(0);
            mHttpsUpgradePref.setSummary(
                    getActivity().getResources().getString(R.string.https_upgrade_option_1));
        } else if (httpsUpgradePref.equals(BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE)) {
            mHttpsUpgradePref.setCheckedIndex(1);
            mHttpsUpgradePref.setSummary(
                    getActivity().getResources().getString(R.string.https_upgrade_option_2));
        } else if (httpsUpgradePref.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
            mHttpsUpgradePref.setCheckedIndex(2);
            mHttpsUpgradePref.setSummary(
                    getActivity().getResources().getString(R.string.https_upgrade_option_3));
        }

        mForgetFirstPartyStoragePref.setChecked(
                BraveShieldsContentSettings.getForgetFirstPartyStoragePref());

        mCanMakePayment.setTitle(
                getActivity()
                        .getResources()
                        .getString(R.string.settings_can_make_payment_toggle_label));
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

        mClearBrowsingDataOnExit.setChecked(
                sharedPreferences.getBoolean(BravePreferenceKeys.BRAVE_CLEAR_ON_EXIT, false));

        mFingerprntLanguagePref.setChecked(
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getBoolean(BravePref.REDUCE_LANGUAGE_ENABLED));
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.isFilterListEnabled(
                    FilterListConstants.COOKIE_LIST_UUID,
                    isEnabled -> {
                        mBlockCookieConsentNoticesPref.setChecked(isEnabled);
                    });
            mFilterListAndroidHandler.isFilterListEnabled(
                    FilterListConstants.SWITCH_TO_APP_UUID,
                    isEnabled -> {
                        mBlockSwitchToAppNoticesPref.setChecked(isEnabled);
                    });
        }
        // Debounce
        if (mDebouncePref != null) {
            mDebouncePref.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.DEBOUNCE_ENABLED));
        }

        // Social blocking
        if (mSocialBlockingGoogle != null) {
            mSocialBlockingGoogle.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.GOOGLE_LOGIN_CONTROL_TYPE));
        }
        if (mSocialBlockingFacebook != null) {
            mSocialBlockingFacebook.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.FB_EMBED_CONTROL_TYPE));
        }
        if (mSocialBlockingTwitter != null) {
            mSocialBlockingTwitter.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.TWITTER_EMBED_CONTROL_TYPE));
        }
        if (mSocialBlockingLinkedin != null) {
            mSocialBlockingLinkedin.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.LINKED_IN_EMBED_CONTROL_TYPE));
        }

        if (mDeAmpPref != null) {
            mDeAmpPref.setChecked(
                    UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.DE_AMP_PREF_ENABLED));
        }

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REQUEST_OTR_TAB)) {
            removePreferenceIfPresent(PREF_REQUEST_OTR);
        } else {
            updateRequestOtrPref();
        }
    }

    private void updateRequestOtrPref() {
        int requestOtrPrefValue =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getInteger(BravePref.REQUEST_OTR_ACTION_OPTION);
        if (requestOtrPrefValue == BraveShieldsContentSettings.ALWAYS) {
            mRequestOtrPref.setCheckedIndex(0);
            mRequestOtrPref.setSummary(
                    getActivity().getResources().getString(R.string.request_otr_option_1));
        } else if (requestOtrPrefValue == BraveShieldsContentSettings.ASK) {
            mRequestOtrPref.setCheckedIndex(1);
            mRequestOtrPref.setSummary(
                    getActivity().getResources().getString(R.string.request_otr_option_2));
        } else if (requestOtrPrefValue == BraveShieldsContentSettings.NEVER) {
            mRequestOtrPref.setCheckedIndex(2);
            mRequestOtrPref.setSummary(
                    getActivity().getResources().getString(R.string.request_otr_option_3));
        }
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
        }
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

/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.lang.System;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Provides information regarding onboarding enabled states.
 */
public class OnboardingPrefManager {
    private static final String PREF_ONBOARDING = "onboarding";
    private static final String PREF_NEXT_ONBOARDING_DATE = "next_onboarding_date";
    private static final String PREF_ONBOARDING_SKIP_COUNT = "onboarding_skip_count";
    public static final String ONBOARDING_TYPE = "onboarding_type";
    public static final String FROM_SETTINGS = "from_settings";

    private static OnboardingPrefManager sInstance;

    private final SharedPreferences mSharedPreferences;

    public static final int NEW_USER_ONBOARDING = 0;
    public static final int EXISTING_USER_REWARDS_OFF_ONBOARDING = 1;
    public static final int EXISTING_USER_REWARDS_ON_ONBOARDING = 2;

    private static boolean isOnboardingNotificationShown;

    public static boolean isNotification;

    private static final List<String> adsAvailableRegions =
            Arrays.asList("US", "CA", "NZ", "IE", "AU", "CA", "FR", "GB", "DE");

    private static final List<String> newAdsAvailableRegions =
            Arrays.asList(); // Add country code for new ad regions in the list

    private static final String GOOGLE = "Google";
    private static final String DUCKDUCKGO = "DuckDuckGo";
    private static final String DUCKDUCKGOLITE = "DuckDuckGo Lite";
    private static final String QWANT = "Qwant";
    private static final String BING = "Bing";
    private static final String STARTPAGE = "StartPage";

    private OnboardingPrefManager() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    /**
     * Returns the singleton instance of OnboardingPrefManager, creating it if needed.
     */
    public static OnboardingPrefManager getInstance() {
        if (sInstance == null) {
            sInstance = new OnboardingPrefManager();
        }
        return sInstance;
    }

    /**
     * Returns the user preference for whether the onboarding is enabled.
     */
    public boolean getPrefOnboardingEnabled() {
        return mSharedPreferences.getBoolean(PREF_ONBOARDING, true);
    }

    /**
     * Sets the user preference for whether the onboarding is enabled.
     */
    public void setPrefOnboardingEnabled(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_ONBOARDING, enabled);
        sharedPreferencesEditor.apply();
    }

    public long getPrefNextOnboardingDate() {
        return mSharedPreferences.getLong(PREF_NEXT_ONBOARDING_DATE, 0);
    }

    public void setPrefNextOnboardingDate(long nextDate) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_ONBOARDING_DATE, nextDate);
        sharedPreferencesEditor.apply();
    }

    public int getPrefOnboardingSkipCount() {
        return mSharedPreferences.getInt(PREF_ONBOARDING_SKIP_COUNT, 0);
    }

    public void setPrefOnboardingSkipCount() {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(
                PREF_ONBOARDING_SKIP_COUNT, getPrefOnboardingSkipCount() + 1);
        sharedPreferencesEditor.apply();
    }

    public boolean isOnboardingNotificationShown() {
        return isOnboardingNotificationShown;
    }

    public void setOnboardingNotificationShown(boolean isShown) {
        isOnboardingNotificationShown = isShown;
    }

    public boolean showOnboardingForSkip() {
        boolean shouldShow = getPrefNextOnboardingDate() == 0
                || (getPrefNextOnboardingDate() > 0
                        && System.currentTimeMillis() > getPrefNextOnboardingDate());
        return shouldShow;
    }

    private boolean shouldShowNewUserOnboarding(Context context) {
        boolean shouldShow = getPrefOnboardingEnabled() && showOnboardingForSkip()
                && PackageUtils.isFirstInstall(context)
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);

        return shouldShow;
    }

    private boolean shouldShowExistingUserOnboardingIfRewardsIsSwitchedOff(Context context) {
        boolean shouldShow = getPrefOnboardingEnabled() && showOnboardingForSkip()
                && isAdsAvailableNewLocale() && !PackageUtils.isFirstInstall(context)
                && !BraveRewardsPanelPopup.isBraveRewardsEnabled()
                && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedProfile())
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);

        return shouldShow;
    }

    private boolean shouldShowExistingUserOnboardingIfRewardsIsSwitchedOn(Context context) {
        boolean shouldShow = getPrefOnboardingEnabled() && showOnboardingForSkip()
                && isAdsAvailableNewLocale() && !PackageUtils.isFirstInstall(context)
                && BraveRewardsPanelPopup.isBraveRewardsEnabled()
                && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedProfile())
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);

        return shouldShow;
    }

    public boolean isAdsAvailable() {
        String locale = BraveAdsNativeHelper.nativeGetLocale();
        String countryCode = BraveAdsNativeHelper.nativeGetCountryCode(locale);
        return adsAvailableRegions.contains(countryCode);
    }

    public boolean isAdsAvailableNewLocale() {
        String locale = BraveAdsNativeHelper.nativeGetLocale();
        String countryCode = BraveAdsNativeHelper.nativeGetCountryCode(locale);
        return newAdsAvailableRegions.contains(countryCode);
    }

    public void showOnboarding(Context context, boolean fromSettings) {
        int onboardingType = -1;
        if (fromSettings) {
            onboardingType = NEW_USER_ONBOARDING;
        } else {
            if (shouldShowNewUserOnboarding(context)) {
                onboardingType = NEW_USER_ONBOARDING;
            } else if (shouldShowExistingUserOnboardingIfRewardsIsSwitchedOff(context)) {
                onboardingType = EXISTING_USER_REWARDS_OFF_ONBOARDING;
            } else if (shouldShowExistingUserOnboardingIfRewardsIsSwitchedOn(context)) {
                onboardingType = EXISTING_USER_REWARDS_ON_ONBOARDING;
            }
        }

        if (onboardingType >= 0) {
            Intent intent = new Intent(context, OnboardingActivity.class);
            intent.putExtra(ONBOARDING_TYPE, onboardingType);
            intent.putExtra(FROM_SETTINGS, fromSettings);
            context.startActivity(intent);
        }
    }

    public void onboardingNotification(Context context, boolean fromSettings) {
        if (!isOnboardingNotificationShown() || fromSettings) {
            BraveOnboardingNotification.showOnboardingNotification(context);

            setOnboardingNotificationShown(true);
        }
    }

    public static Map<String, SearchEngineEnum> searchEngineMap =
            new HashMap<String, SearchEngineEnum>() {
                {
                    put(GOOGLE, SearchEngineEnum.GOOGLE);
                    put(DUCKDUCKGO, SearchEngineEnum.DUCKDUCKGO);
                    put(DUCKDUCKGOLITE, SearchEngineEnum.DUCKDUCKGOLITE);
                    put(QWANT, SearchEngineEnum.QWANT);
                    put(BING, SearchEngineEnum.BING);
                    put(STARTPAGE, SearchEngineEnum.STARTPAGE);
                }
            };
}

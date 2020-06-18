/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.PackageUtils;

import java.lang.System;
import java.util.HashMap;
import java.util.Map;

/**
 * Provides information regarding onboarding enabled states.
 */
public class OnboardingPrefManager {
    private static final String PREF_ONBOARDING = "onboarding";
    private static final String PREF_NEXT_ONBOARDING_DATE = "next_onboarding_date";
    private static final String PREF_ONBOARDING_FOR_SKIP = "onboarding_for_skip";
    private static final String PREF_ONBOARDING_SKIP_COUNT = "onboarding_skip_count";
    private static final String PREF_SEARCH_ENGINE_ONBOARDING = "search_engine_onboarding";
    public static final String ONBOARDING_TYPE = "onboarding_type";
    public static final String FROM_SETTINGS = "from_settings";

    private static OnboardingPrefManager sInstance;

    private final SharedPreferences mSharedPreferences;

    public static final int NEW_USER_ONBOARDING = 0;
    public static final int EXISTING_USER_REWARDS_OFF_ONBOARDING = 1;
    public static final int EXISTING_USER_REWARDS_ON_ONBOARDING = 2;

    private static boolean isOnboardingNotificationShown;

    public static boolean isNotification;

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
     * Returns the user preference for whether the onboarding is shown.
     */
    public boolean isOnboardingShown() {
        return mSharedPreferences.getBoolean(PREF_ONBOARDING, false);
    }

    /**
     * Sets the user preference for whether the onboarding is shown.
     */
    public void setOnboardingShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_ONBOARDING, isShown);
        sharedPreferencesEditor.apply();
    }

    public boolean isOnboardingNotificationShown() {
        return isOnboardingNotificationShown;
    }

    public void setOnboardingNotificationShown(boolean isShown) {
        isOnboardingNotificationShown = isShown;
    }

    public boolean hasSearchEngineOnboardingShown() {
        return mSharedPreferences.getBoolean(PREF_SEARCH_ENGINE_ONBOARDING, false);
    }

    public void setSearchEngineOnboardingShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_SEARCH_ENGINE_ONBOARDING, isShown);
        sharedPreferencesEditor.apply();
    }

    public long getNextOnboardingDate() {
        return mSharedPreferences.getLong(PREF_NEXT_ONBOARDING_DATE, 0);
    }

    public void setNextOnboardingDate(long nextDate) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_ONBOARDING_DATE, nextDate);
        sharedPreferencesEditor.apply();
    }

    public boolean hasOnboardingShownForSkip() {
        return mSharedPreferences.getBoolean(PREF_ONBOARDING_FOR_SKIP, false);
    }

    public void setOnboardingShownForSkip(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_ONBOARDING_FOR_SKIP, isShown);
        sharedPreferencesEditor.apply();
    }

    public boolean showOnboardingForSkip() {
        boolean shouldShow = !hasOnboardingShownForSkip()
                             && (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS) && !BraveRewardsPanelPopup.isBraveRewardsEnabled())
                             && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedProfile())
                             && (getNextOnboardingDate() > 0 && System.currentTimeMillis() > getNextOnboardingDate());
        return shouldShow;
    }

    // private boolean shouldShowNewUserOnboarding(Context context) {
    //     boolean shouldShow = getPrefOnboardingEnabled() && showOnboardingForSkip()
    //                          && PackageUtils.isFirstInstall(context)
    //                          && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);

    //     return shouldShow;
    // }

    // private boolean shouldShowExistingUserOnboardingIfRewardsIsSwitchedOff(Context context) {
    //     boolean shouldShow = getPrefOnboardingEnabled() && showOnboardingForSkip()
    //                          && isAdsAvailableNewLocale() && !PackageUtils.isFirstInstall(context)
    //                          && !BraveRewardsPanelPopup.isBraveRewardsEnabled()
    //                          && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedProfile())
    //                          && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);

    //     return shouldShow;
    // }

    // private boolean shouldShowExistingUserOnboardingIfRewardsIsSwitchedOn(Context context) {
    //     boolean shouldShow = getPrefOnboardingEnabled() && showOnboardingForSkip()
    //                          && isAdsAvailableNewLocale() && !PackageUtils.isFirstInstall(context)
    //                          && BraveRewardsPanelPopup.isBraveRewardsEnabled()
    //                          && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedProfile())
    //                          && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);

    //     return shouldShow;
    // }

    public boolean isAdsAvailable() {
        return BraveAdsNativeHelper.nativeIsSupportedLocale(Profile.getLastUsedProfile());
    }

    // public boolean isAdsAvailableNewLocale() {
    //     return BraveAdsNativeHelper.nativeIsNewlySupportedLocale(Profile.getLastUsedProfile());
    // }

    public void showOnboarding(Context context) {
        // int onboardingType = -1;
        // if (fromSettings) {
        //     onboardingType = NEW_USER_ONBOARDING;
        // } else {
        //     if (shouldShowNewUserOnboarding(context)) {
        //         onboardingType = NEW_USER_ONBOARDING;
        //     } else if (shouldShowExistingUserOnboardingIfRewardsIsSwitchedOff(context)) {
        //         onboardingType = EXISTING_USER_REWARDS_OFF_ONBOARDING;
        //     } else if (shouldShowExistingUserOnboardingIfRewardsIsSwitchedOn(context)) {
        //         onboardingType = EXISTING_USER_REWARDS_ON_ONBOARDING;
        //     }
        // }

        // if (onboardingType >= 0) {
        //     Intent intent = new Intent(context, OnboardingActivity.class);
        //     intent.putExtra(ONBOARDING_TYPE, onboardingType);
        //     intent.putExtra(FROM_SETTINGS, fromSettings);
        //     context.startActivity(intent);
        // }
        Intent intent = new Intent(context, OnboardingActivity.class);
        context.startActivity(intent);
    }

    public void onboardingNotification(Context context) {
        if (!isOnboardingNotificationShown()) {
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

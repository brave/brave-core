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
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.PackageUtils;

import java.lang.System;
import java.util.HashMap;
import java.util.Map;

/**
 * Provides information regarding onboarding.
 */
public class OnboardingPrefManager {
    private static final String PREF_ONBOARDING = "onboarding";
    private static final String PREF_CROSS_PROMO_MODAL = "cross_promo_modal";
    private static final String PREF_ONBOARDING_V2 = "onboarding_v2";
    private static final String PREF_NEXT_ONBOARDING_DATE = "next_onboarding_date";
    private static final String PREF_NEXT_CROSS_PROMO_MODAL_DATE = "next_cross_promo_modal_date";
    private static final String PREF_ONBOARDING_FOR_SKIP = "onboarding_for_skip";
    private static final String PREF_ONBOARDING_SKIP_COUNT = "onboarding_skip_count";
    private static final String PREF_SEARCH_ENGINE_ONBOARDING = "search_engine_onboarding";
    private static final String PREF_SHIELDS_TOOLTIP = "shields_tooltip";
    public static final String PREF_BRAVE_STATS = "brave_stats";
    public static final String PREF_BRAVE_STATS_NOTIFICATION = "brave_stats_notification";
    public static final String ONBOARDING_TYPE = "onboarding_type";
    public static final String FROM_NOTIFICATION = "from_notification";
    public static final String FROM_STATS = "from_stats";
    public static final String ONE_TIME_NOTIFICATION = "one_time_notification";
    public static final String ADS_TRACKERS_NOTIFICATION = "ads_trackers_notification";
    public static final String DATA_SAVED_NOTIFICATION = "data_saved_notification";
    public static final String TIME_SAVED_NOTIFICATION = "time_saved_notification";
    public static final String SHOW_BADGE_ANIMATION = "show_badge_animation";

    private static OnboardingPrefManager sInstance;

    private final SharedPreferences mSharedPreferences;

    public static final int NEW_USER_ONBOARDING = 0;
    public static final int EXISTING_USER_REWARDS_OFF_ONBOARDING = 1;
    public static final int EXISTING_USER_REWARDS_ON_ONBOARDING = 2;

    private static boolean isOnboardingNotificationShown;

    public static boolean isNotification;

    private static final String GOOGLE = "Google";
    public static final String DUCKDUCKGO = "DuckDuckGo";
    private static final String DUCKDUCKGOLITE = "DuckDuckGo Lite";
    private static final String QWANT = "Qwant";
    private static final String BING = "Bing";
    private static final String STARTPAGE = "StartPage";
    private static final String YAHOO = "Yahoo";

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

    /**
     * Returns the user preference for whether the onboarding is shown.
     */
    public boolean isNewOnboardingShown() {
        return mSharedPreferences.getBoolean(PREF_ONBOARDING_V2, false);
    }

    /**
     * Sets the user preference for whether the onboarding is shown.
     */
    public void setNewOnboardingShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_ONBOARDING_V2, isShown);
        sharedPreferencesEditor.apply();
    }

    public boolean isOnboardingNotificationShown() {
        return isOnboardingNotificationShown;
    }

    public boolean isBraveStatsEnabled() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_STATS, false);
    }

    public void setBraveStatsEnabled(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_STATS, enabled);
        sharedPreferencesEditor.apply();
    }

    public boolean isBraveStatsNotificationEnabled() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_STATS_NOTIFICATION, true);
    }

    public void setBraveStatsNotificationEnabled(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_STATS_NOTIFICATION, enabled);
        sharedPreferencesEditor.apply();
    }

    public long getPrefNextOnboardingDate() {
        return mSharedPreferences.getLong(PREF_NEXT_ONBOARDING_DATE, 0);
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

    public boolean hasShieldsTooltipShown() {
        return mSharedPreferences.getBoolean(PREF_SHIELDS_TOOLTIP, false);
    }

    public void setShieldsTooltipShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_SHIELDS_TOOLTIP, isShown);
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

    public boolean showOnboardingForSkip(Context context) {
        boolean shouldShow = PackageUtils.isFirstInstall(context)
                             && !hasOnboardingShownForSkip()
                             && (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS) && !UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.ENABLED))
                             && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedRegularProfile())
                             && (getNextOnboardingDate() > 0 && System.currentTimeMillis() > getNextOnboardingDate());
        return shouldShow;
    }

    public boolean isAdsAvailable() {
        return BraveAdsNativeHelper.nativeIsSupportedLocale(Profile.getLastUsedRegularProfile());
    }

    public void showOnboarding(Context context) {
        Intent intent = new Intent(context, OnboardingActivity.class);
        context.startActivity(intent);
    }

    public void onboardingNotification(Context context) {
        if (!isOnboardingNotificationShown()) {
            BraveOnboardingNotification.showOnboardingNotification(context);
            setOnboardingNotificationShown(true);
        }
    }

    private long getNextCrossPromoModalDate() {
        return mSharedPreferences.getLong(PREF_NEXT_CROSS_PROMO_MODAL_DATE, 0);
    }

    public void setNextCrossPromoModalDate(long nextDate) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_CROSS_PROMO_MODAL_DATE, nextDate);
        sharedPreferencesEditor.apply();
    }

    public void setCrossPromoModalShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_CROSS_PROMO_MODAL, isShown);
        sharedPreferencesEditor.apply();
    }

    private boolean hasCrossPromoModalShown() {
        return mSharedPreferences.getBoolean(PREF_CROSS_PROMO_MODAL, false);
    }

    public boolean showCrossPromoModal() {
        boolean shouldShow = !hasCrossPromoModalShown()
                             && (getNextCrossPromoModalDate() > 0
                                 && System.currentTimeMillis() > getNextCrossPromoModalDate());
        return shouldShow;
    }

    public static Map<String, SearchEngineEnum> searchEngineMap =
    new HashMap<String, SearchEngineEnum>() {
        {
            put(GOOGLE, SearchEngineEnum.GOOGLE);
            put(YAHOO, SearchEngineEnum.YAHOO);
            put(DUCKDUCKGO, SearchEngineEnum.DUCKDUCKGO);
            put(DUCKDUCKGOLITE, SearchEngineEnum.DUCKDUCKGOLITE);
            put(QWANT, SearchEngineEnum.QWANT);
            put(BING, SearchEngineEnum.BING);
            put(STARTPAGE, SearchEngineEnum.STARTPAGE);
        }
    };

    public boolean isFromNotification() {
        return mSharedPreferences.getBoolean(FROM_NOTIFICATION, false);
    }

    public void setFromNotification(boolean isFromNotification) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(FROM_NOTIFICATION, isFromNotification);
        sharedPreferencesEditor.apply();
    }

    public boolean isOneTimeNotificationStarted() {
        return mSharedPreferences.getBoolean(ONE_TIME_NOTIFICATION, false);
    }

    public void setOneTimeNotificationStarted(boolean isOneTimeNotificationStarted) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(ONE_TIME_NOTIFICATION, isOneTimeNotificationStarted);
        sharedPreferencesEditor.apply();
    }

    public boolean isAdsTrackersNotificationStarted() {
        return mSharedPreferences.getBoolean(ADS_TRACKERS_NOTIFICATION, false);
    }

    public void setAdsTrackersNotificationStarted(boolean isAdsTrackersNotificationStarted) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(ADS_TRACKERS_NOTIFICATION, isAdsTrackersNotificationStarted);
        sharedPreferencesEditor.apply();
    }

    public boolean isDataSavedNotificationStarted() {
        return mSharedPreferences.getBoolean(DATA_SAVED_NOTIFICATION, false);
    }

    public void setDataSavedNotificationStarted(boolean isDataSavedNotificationStarted) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(DATA_SAVED_NOTIFICATION, isDataSavedNotificationStarted);
        sharedPreferencesEditor.apply();
    }

    public boolean isTimeSavedNotificationStarted() {
        return mSharedPreferences.getBoolean(TIME_SAVED_NOTIFICATION, false);
    }

    public void setTimeSavedNotificationStarted(boolean isTimeSavedNotificationStarted) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(TIME_SAVED_NOTIFICATION, isTimeSavedNotificationStarted);
        sharedPreferencesEditor.apply();
    }

    public boolean shouldShowBadgeAnimation() {
        return mSharedPreferences.getBoolean(SHOW_BADGE_ANIMATION, true);
    }

    public void setShowBadgeAnimation(boolean shouldShowBadgeAnimation) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(SHOW_BADGE_ANIMATION, shouldShowBadgeAnimation);
        sharedPreferencesEditor.apply();
    }
}

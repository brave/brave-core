/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.components.user_prefs.UserPrefs;

import java.lang.System;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

/**
 * Provides information regarding onboarding.
 */
public class OnboardingPrefManager {
    private static final String PREF_ONBOARDING = "onboarding";
    private static final String PREF_P3A_ONBOARDING = "p3a_onboarding";
    private static final String PREF_CROSS_PROMO_MODAL = "cross_promo_modal";
    private static final String PREF_ONBOARDING_V2 = "onboarding_v2";
    private static final String PREF_NEXT_ONBOARDING_DATE = "next_onboarding_date";
    private static final String PREF_NEXT_CROSS_PROMO_MODAL_DATE = "next_cross_promo_modal_date";
    private static final String PREF_SEARCH_ENGINE_ONBOARDING = "search_engine_onboarding";
    private static final String PREF_SHOW_DEFAULT_BROWSER_MODAL_AFTER_P3A =
            "show_default_browser_modal_after_p3a";
    public static final String PREF_BRAVE_STATS = "brave_stats";
    public static final String PREF_BRAVE_STATS_NOTIFICATION = "brave_stats_notification";
    public static final String FROM_NOTIFICATION = "from_notification";
    public static final String FROM_STATS = "from_stats";
    public static final String ONE_TIME_NOTIFICATION = "one_time_notification";
    public static final String DORMANT_USERS_NOTIFICATION = "dormant_users_notification";
    public static final String SHOW_BADGE_ANIMATION = "show_badge_animation";
    public static final String PREF_DORMANT_USERS_ENGAGEMENT = "dormant_users_engagement";
    private static final String PREF_SHOW_SEARCHBOX_TOOLTIP = "show_searchbox_tooltip";
    private static final String PREF_P3A_CRASH_REPORTING_MESSAGE_SHOWN =
            "p3a_crash_reporting_message_shown";

    private static OnboardingPrefManager sInstance;

    private final SharedPreferences mSharedPreferences;

    public static final int NEW_USER_ONBOARDING = 0;
    public static final int EXISTING_USER_REWARDS_OFF_ONBOARDING = 1;
    public static final int EXISTING_USER_REWARDS_ON_ONBOARDING = 2;

    private static boolean isOnboardingNotificationShown;

    public static boolean isNotification;

    private static final String GOOGLE = "Google";
    public static final String BRAVE = "Brave";
    public static final String DUCKDUCKGO = "DuckDuckGo";
    private static final String QWANT = "Qwant";
    private static final String BING = "Bing";
    private static final String STARTPAGE = "Startpage";
    public static final String YANDEX = "Yandex";
    public static final String ECOSIA = "Ecosia";

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

    public boolean isOnboardingSearchBoxTooltip() {
        return mSharedPreferences.getBoolean(PREF_SHOW_SEARCHBOX_TOOLTIP, false);
    }

    public void setOnboardingSearchBoxTooltip(boolean shouldTooltipShow) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_SHOW_SEARCHBOX_TOOLTIP, shouldTooltipShow);
        sharedPreferencesEditor.apply();
    }

    public boolean isP3aCrashReportingMessageShown() {
        return mSharedPreferences.getBoolean(PREF_P3A_CRASH_REPORTING_MESSAGE_SHOWN, false);
    }

    public void setP3aCrashReportingMessageShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_P3A_CRASH_REPORTING_MESSAGE_SHOWN, isShown);
        sharedPreferencesEditor.apply();
    }

    /**
     * Returns the user preference for whether the onboarding is shown.
     */
    public boolean isP3aOnboardingShown() {
        return mSharedPreferences.getBoolean(PREF_P3A_ONBOARDING, false);
    }

    /**
     * Sets the user preference for whether the onboarding is shown.
     */
    public void setP3aOnboardingShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_P3A_ONBOARDING, isShown);
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

    public boolean hasSearchEngineOnboardingShown() {
        return mSharedPreferences.getBoolean(PREF_SEARCH_ENGINE_ONBOARDING, false);
    }

    public void setSearchEngineOnboardingShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_SEARCH_ENGINE_ONBOARDING, isShown);
        sharedPreferencesEditor.apply();
    }

    public boolean isAdsAvailable() {
        return BraveAdsNativeHelper.nativeIsSupportedLocale(Profile.getLastUsedRegularProfile());
    }

    public void showOnboarding(Context context) {
        Intent intent = new Intent(context, OnboardingActivity.class);
        context.startActivity(intent);
    }

    public boolean isOnboardingNotificationShown() {
        return isOnboardingNotificationShown;
    }

    public void setOnboardingNotificationShown(boolean isShown) {
        isOnboardingNotificationShown = isShown;
    }

    public void onboardingNotification() {
        if (!isOnboardingNotificationShown()) {
            BraveOnboardingNotification.showOnboardingNotification();
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
            put(BRAVE, SearchEngineEnum.BRAVE);
            put(DUCKDUCKGO, SearchEngineEnum.DUCKDUCKGO);
            put(QWANT, SearchEngineEnum.QWANT);
            put(BING, SearchEngineEnum.BING);
            put(STARTPAGE, SearchEngineEnum.STARTPAGE);
            put(YANDEX, SearchEngineEnum.YANDEX);
            put(ECOSIA, SearchEngineEnum.ECOSIA);
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

    public boolean isDormantUsersEngagementEnabled() {
        return mSharedPreferences.getBoolean(PREF_DORMANT_USERS_ENGAGEMENT, false);
    }

    public boolean isDormantUsersNotificationsStarted() {
        return mSharedPreferences.getBoolean(DORMANT_USERS_NOTIFICATION, false);
    }

    public void setDormantUsersNotificationsStarted(boolean isDormantUsersNotificationsStarted) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(
                DORMANT_USERS_NOTIFICATION, isDormantUsersNotificationsStarted);
        sharedPreferencesEditor.apply();
    }

    public long getDormantUsersNotificationTime(String notificationType) {
        return mSharedPreferences.getLong(notificationType, 0);
    }

    public void setDormantUsersNotificationTime(String notificationType, long timeInMilliseconds) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(notificationType, timeInMilliseconds);
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

    public void setDormantUsersPrefs() {
        setDormantUsersNotificationTime(
                RetentionNotificationUtil.DORMANT_USERS_DAY_14, setTimeInMillis(14 * 24 * 60));
        setDormantUsersNotificationTime(
                RetentionNotificationUtil.DORMANT_USERS_DAY_25, setTimeInMillis(25 * 24 * 60));
        setDormantUsersNotificationTime(
                RetentionNotificationUtil.DORMANT_USERS_DAY_40, setTimeInMillis(40 * 24 * 60));
    }

    private long setTimeInMillis(int timeInMinutes) {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(System.currentTimeMillis());
        calendar.add(Calendar.MINUTE, timeInMinutes);

        Date date = calendar.getTime();
        return date.getTime();
    }
}

/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.Calendar;
import java.util.Date;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;

public class RateUtils {

    private static RateUtils sInstance;

    private final SharedPreferences mSharedPreferences;

    public static final String FROM_SETTINGS = "from_settings";

    private static final int DAYS_60 = 60;
    private static final int APP_OPEN_14 = 14;
    private static final int APP_OPEN_41 = 41;
    private static final int APP_OPEN_121 = 121;

    private static final String PREF_RATE = "rate";
    private static final String PREF_NEXT_RATE_DATE = "next_rate_date";
    private static final String PREF_NEXT_APP_OPEN_COUNT = "next_app_open_count";

    private RateUtils(Context context) {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    /**
     * Returns the singleton instance of RateUtils, creating it if needed.
     */
    public static RateUtils getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new RateUtils(context);
        }
        return sInstance;
    }

    /**
     * Returns the user preference for whether the rate is enabled.
     */
    public boolean getPrefRateEnabled() {
        return mSharedPreferences.getBoolean(PREF_RATE, false);
    }

    /**
     * Sets the user preference for whether the rate is enabled.
     */
    public void setPrefRateEnabled(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_RATE, enabled);
        sharedPreferencesEditor.apply();
    }

    public long getPrefNextRateDate() {
        return mSharedPreferences.getLong(PREF_NEXT_RATE_DATE, 0);
    }

    public void setPrefNextRateDate() {
        Calendar calender = Calendar.getInstance();
        calender.setTime(new Date());
        calender.add(Calendar.DATE, DAYS_60);

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_RATE_DATE, calender.getTimeInMillis());
        sharedPreferencesEditor.apply();
    }

    public int getPrefNextAppOpenCount() {
        return mSharedPreferences.getInt(PREF_NEXT_APP_OPEN_COUNT, 0);
    }

    public void setPrefNextAppOpenCount() {
        int currentAppOpenCount = getPrefNextAppOpenCount();
        int nextAppOpenCount;

        if (currentAppOpenCount == 0) {
            nextAppOpenCount = APP_OPEN_14;
        } else if (currentAppOpenCount == APP_OPEN_14) {
            nextAppOpenCount = APP_OPEN_41;
        } else if (currentAppOpenCount == APP_OPEN_41) {
            nextAppOpenCount = APP_OPEN_121;
        } else {
            nextAppOpenCount = APP_OPEN_121;
        }

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(PREF_NEXT_APP_OPEN_COUNT, nextAppOpenCount);
        sharedPreferencesEditor.apply();
    }

    public boolean shouldShowRateDialog() {
        int appOpenCount = SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT);

        return (System.currentTimeMillis() > getPrefNextRateDate()
            && appOpenCount >= getPrefNextAppOpenCount()
            && getPrefRateEnabled());
    }

    public void setNextRateDateAndCount() {
        setPrefNextAppOpenCount();
        setPrefNextRateDate();
    }
}

/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;

import java.time.LocalTime;
import java.util.Calendar;
import java.util.Date;

public class RateUtils {

    private static RateUtils sInstance;

    private final SharedPreferences mSharedPreferences;

    private static final String PREF_RATE = "rate";
    private static final String PREF_NEXT_RATE_DATE = "next_rate_date";
    private static final String PREF_RATE_COUNT = "rate_count";
    private static final String PREF_APP_OPEN_COUNT = "app_open_count";

    private static final String MyPREFERENCES = "MyPrefs";

    private RateUtils(Context context) {
        mSharedPreferences = context.getSharedPreferences(MyPREFERENCES, Context.MODE_PRIVATE);
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
        return mSharedPreferences.getBoolean(PREF_RATE, true);
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

    public void setPrefNextRateDate(long nextDate) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_RATE_DATE, nextDate);
        sharedPreferencesEditor.apply();
    }

    public int getPrefRateCount() {
        return mSharedPreferences.getInt(PREF_RATE_COUNT, -1);
    }

    public void setPrefRateCount() {
        if(RateDays.DAYS_365.count != getPrefRateCount()) {
            SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
            sharedPreferencesEditor.putInt(PREF_RATE_COUNT, getPrefRateCount() + 1);
            sharedPreferencesEditor.apply();
        }
    }

    public int getPrefAppOpenCount() {
        return mSharedPreferences.getInt(PREF_APP_OPEN_COUNT, 0);
    }

    public void setPrefAppOpenCount() {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(PREF_APP_OPEN_COUNT, getPrefAppOpenCount() + 1);
        sharedPreferencesEditor.apply();
    }

    public boolean shouldShowRate() {

//        RateDays rateDays = RateDays.valueOfIndex(getPrefRateCount());
//
//        LocalTime morning10 = LocalTime.parse("10:00:00");
//        LocalTime afternoon1 = LocalTime.parse("13:00:00");
//
//        LocalTime timeNow = LocalTime.now();
//
//        return System.currentTimeMillis() > getPrefNextRateDate() &&
//                getPrefAppOpenCount() >= rateDays.count &&
//                timeNow.isAfter(morning10) && timeNow.isBefore(afternoon1);
        return true;
    }

    public void setNextRateDateAndCount() {
        setPrefRateCount();

        Calendar calender = Calendar.getInstance();
        calender.setTime(new Date());
        calender.add(Calendar.DATE, RateDays.valueOfIndex(getPrefRateCount()).count);

        setPrefNextRateDate(calender.getTimeInMillis());
    }
}

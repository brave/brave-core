/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

import java.util.Calendar;
import java.util.Date;

public class RateUtils {

    private static RateUtils sInstance;

    private final SharedPreferences mSharedPreferences;

    public static final String FROM_SETTINGS = "from_settings";

    private static final int DAYS_30 = 30;
    private static final int APP_OPEN_5 = 5;
    private static final int BOOKMARKS_COUNT = 5;
    private static final int LAST_7_DAYS = 7;

    private static final String PREF_RATE = "rate";
    private static final String PREF_NEXT_RATE_DATE = "next_rate_date";
    private static final String PREF_ADDED_BOOKMARK_COUNT = "added_bookmark_count";

    private static final String PREF_LAST_SESSION_SHOWN = "last_session_shown";

    private static final String PREF_LAST_TIME_APP_USED_DATE1 = "last_time_app_used_date1";
    private static final String PREF_LAST_TIME_APP_USED_DATE2 = "last_time_app_used_date2";
    private static final String PREF_LAST_TIME_APP_USED_DATE3 = "last_time_app_used_date3";
    private static final String PREF_LAST_TIME_APP_USED_DATE4 = "last_time_app_used_date4";

    private long lastTimeUsedDate1;
    private long lastTimeUsedDate2;
    private long lastTimeUsedDate3;
    private long lastTimeUsedDate4;

    private RateUtils() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    /**
     * Returns the singleton instance of RateUtils, creating it if needed.
     */
    public static RateUtils getInstance() {
        if (sInstance == null) {
            sInstance = new RateUtils();
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
        calender.add(Calendar.DATE, DAYS_30);

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_RATE_DATE, calender.getTimeInMillis());
        sharedPreferencesEditor.apply();
    }

    public int getPrefAddedBookmarkCount() {
        return mSharedPreferences.getInt(PREF_ADDED_BOOKMARK_COUNT, 0);
    }

    public void setPrefAddedBookmarkCount() {
        int currentBookmarkCount = getPrefAddedBookmarkCount();

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(PREF_ADDED_BOOKMARK_COUNT, currentBookmarkCount + 1);
        sharedPreferencesEditor.apply();
    }

    public boolean isLastSessionShown() {
        return mSharedPreferences.getBoolean(PREF_LAST_SESSION_SHOWN, false);
    }

    public void setLastSessionShown(boolean shown) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_LAST_SESSION_SHOWN, shown);
        sharedPreferencesEditor.apply();
    }

    /**
     *
     * 1. every 30 days
     * 2. app opened 5 days or more
     * 3. Last 7 days 4 days used not to be consecutive
     * 4. Any one of the following true
     *      i.  User has added at least 5 bookmarks.
     *      ii. User has set Brave as default.
     *      iii.User has paid for the VPN subscription.
     * */
    public boolean shouldShowRateDialog(Context context) {
        return mainCriteria() && anyOneSubCriteria(context);
    }

    private boolean mainCriteria() {
        int appOpenCount =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT);

        return (System.currentTimeMillis() > getPrefNextRateDate() && appOpenCount >= APP_OPEN_5
                && getPrefRateEnabled() && is4DaysUsedLast7Days());
    }

    private boolean anyOneSubCriteria(Context context) {
        return BraveVpnPrefUtils.isSubscriptionPurchase()
                || BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(context)
                || getPrefAddedBookmarkCount() >= BOOKMARKS_COUNT;
    }

    public void setTodayDate() {
        long today = new Date().getTime();
        lastTimeUsedDate1 = mSharedPreferences.getLong(PREF_LAST_TIME_APP_USED_DATE1, 0L);
        lastTimeUsedDate2 = mSharedPreferences.getLong(PREF_LAST_TIME_APP_USED_DATE2, 0L);
        lastTimeUsedDate3 = mSharedPreferences.getLong(PREF_LAST_TIME_APP_USED_DATE3, 0L);
        lastTimeUsedDate4 = mSharedPreferences.getLong(PREF_LAST_TIME_APP_USED_DATE4, 0L);

        if (dayDifference(today, lastTimeUsedDate1) == 0) {
            return;
        }

        lastTimeUsedDate4 = lastTimeUsedDate3;
        lastTimeUsedDate3 = lastTimeUsedDate2;
        lastTimeUsedDate2 = lastTimeUsedDate1;

        lastTimeUsedDate1 = today;

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_LAST_TIME_APP_USED_DATE1, lastTimeUsedDate1);
        sharedPreferencesEditor.putLong(PREF_LAST_TIME_APP_USED_DATE2, lastTimeUsedDate2);
        sharedPreferencesEditor.putLong(PREF_LAST_TIME_APP_USED_DATE3, lastTimeUsedDate3);
        sharedPreferencesEditor.putLong(PREF_LAST_TIME_APP_USED_DATE4, lastTimeUsedDate4);
        sharedPreferencesEditor.apply();
    }

    private boolean is4DaysUsedLast7Days() {
        if (dayDifference(lastTimeUsedDate1, lastTimeUsedDate2) <= LAST_7_DAYS
                && dayDifference(lastTimeUsedDate1, lastTimeUsedDate3) <= LAST_7_DAYS
                && dayDifference(lastTimeUsedDate1, lastTimeUsedDate4) <= LAST_7_DAYS)
            return true;
        return false;
    }

    private long dayDifference(long date1, long date2) {
        long difference = date1 - date2;
        return (difference / (1000 * 60 * 60 * 24)) % 365;
    }

    /**
     * This opens app page in playstore
     * if it fails open app playstore page link in browser
     * */
    public void openPlaystore(Context context) {
        final Uri marketUri = Uri.parse("market://details?id=" + context.getPackageName());
        try {
            context.startActivity(new Intent(Intent.ACTION_VIEW, marketUri));
        } catch (android.content.ActivityNotFoundException ex) {
            openReviewLink(context);
        }
    }

    private void openReviewLink(Context context) {
        Intent webIntent = new Intent(Intent.ACTION_VIEW,
                Uri.parse("https://play.google.com/store/apps/details?id="
                        + context.getPackageName()));
        context.startActivity(webIntent);
    }
}

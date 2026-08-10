/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ResettersForTesting;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

import java.util.Calendar;
import java.util.Date;

public class RateUtils {

    private static RateUtils sInstance;

    public static final String FROM_SETTINGS = "from_settings";

    private static final int DAYS_30 = 30;
    private static final int APP_OPEN_5 = 5;
    private static final int BOOKMARKS_COUNT = 5;
    private static final int LAST_7_DAYS = 7;

    private static final String PREF_RATE = "rate";
    private static final String PREF_NEXT_RATE_DATE = "next_rate_date";
    private static final String QA_FORCE_RATE_DIALOG = "qa_force_rate_dialog";
    private static final String PREF_ADDED_BOOKMARK_COUNT = "added_bookmark_count";

    private static final String PREF_LAST_SESSION_SHOWN = "last_session_shown";

    private static final String PREF_LAST_TIME_APP_USED_DATE1 = "last_time_app_used_date1";
    private static final String PREF_LAST_TIME_APP_USED_DATE2 = "last_time_app_used_date2";
    private static final String PREF_LAST_TIME_APP_USED_DATE3 = "last_time_app_used_date3";
    private static final String PREF_LAST_TIME_APP_USED_DATE4 = "last_time_app_used_date4";

    private long mLastTimeUsedDate1;
    private long mLastTimeUsedDate2;
    private long mLastTimeUsedDate3;
    private long mLastTimeUsedDate4;

    private RateUtils() {}

    /** Returns the singleton instance of RateUtils, creating it if needed. */
    public static RateUtils getInstance() {
        if (sInstance == null) {
            sInstance = new RateUtils();
        }
        return sInstance;
    }

    /**
     * Drops the singleton so the next {@link #getInstance()} starts from a clean state. The
     * preferences live in shared preferences and are unaffected; this only clears the cached
     * app-usage dates held on the instance.
     */
    public static void resetForTesting() {
        sInstance = null;
        ResettersForTesting.register(() -> sInstance = null);
    }

    /** Returns the user preference for whether the rate is enabled. */
    public boolean getPrefRateEnabled() {
        return ChromeSharedPreferences.getInstance().readBoolean(PREF_RATE, false);
    }

    /** Sets the user preference for whether the rate is enabled. */
    public void setPrefRateEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance().writeBoolean(PREF_RATE, enabled);
    }

    /** Returns whether the user opted out of the rating prompt with "Don't show again". */
    public boolean getPrefRateDontShowAgain() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_RATE_DONT_SHOW_AGAIN, false);
    }

    /**
     * Records that the user opted out of the rating prompt with "Don't show again". This only
     * suppresses the automatic prompt; the dialog can still be opened from settings.
     */
    public void setPrefRateDontShowAgain(boolean dontShowAgain) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_RATE_DONT_SHOW_AGAIN, dontShowAgain);
    }

    public long getPrefNextRateDate() {
        return ChromeSharedPreferences.getInstance().readLong(PREF_NEXT_RATE_DATE, 0);
    }

    public void setPrefNextRateDate() {
        Calendar calender = Calendar.getInstance();
        calender.setTime(new Date());
        calender.add(Calendar.DATE, DAYS_30);

        ChromeSharedPreferences.getInstance()
                .writeLong(PREF_NEXT_RATE_DATE, calender.getTimeInMillis());
    }

    public int getPrefAddedBookmarkCount() {
        return ChromeSharedPreferences.getInstance().readInt(PREF_ADDED_BOOKMARK_COUNT, 0);
    }

    public void setPrefAddedBookmarkCount() {
        int currentBookmarkCount = getPrefAddedBookmarkCount();

        ChromeSharedPreferences.getInstance()
                .writeInt(PREF_ADDED_BOOKMARK_COUNT, currentBookmarkCount + 1);
    }

    public boolean isLastSessionShown() {
        return ChromeSharedPreferences.getInstance().readBoolean(PREF_LAST_SESSION_SHOWN, false);
    }

    public void setLastSessionShown(boolean shown) {
        ChromeSharedPreferences.getInstance().writeBoolean(PREF_LAST_SESSION_SHOWN, shown);
    }

    /**
     * Whether the rating prompt should be shown. All of the following must hold:
     *
     * <ul>
     *   <li>User has not opted out with "Don't show again"
     *   <li>Every 30 days
     *   <li>App opened 5 days or more
     *   <li>4 of the last 7 days used, not necessarily consecutive
     *   <li>Any one of:
     *       <ul>
     *         <li>User has added at least 5 bookmarks
     *         <li>User has set Brave as default
     *         <li>User has paid for the VPN subscription
     *       </ul>
     * </ul>
     */
    public boolean shouldShowRateDialog(Context context) {
        if (getPrefRateDontShowAgain()) {
            return false;
        }
        return isQaForceRateDialogEnabled() || (mainCriteria() && anyOneSubCriteria(context));
    }

    /**
     * QA-only override from Developer options: shows the prompt regardless of the usage criteria.
     * "Don't show again" deliberately still wins over it, so the opt-out stays testable with this
     * switched on; the prompt can always be reopened from settings to clear that preference.
     */
    private boolean isQaForceRateDialogEnabled() {
        return ChromeSharedPreferences.getInstance().readBoolean(QA_FORCE_RATE_DIALOG, false);
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
        SharedPreferencesManager sharedPreferencesManager = ChromeSharedPreferences.getInstance();
        long today = new Date().getTime();
        mLastTimeUsedDate1 = sharedPreferencesManager.readLong(PREF_LAST_TIME_APP_USED_DATE1, 0L);
        mLastTimeUsedDate2 = sharedPreferencesManager.readLong(PREF_LAST_TIME_APP_USED_DATE2, 0L);
        mLastTimeUsedDate3 = sharedPreferencesManager.readLong(PREF_LAST_TIME_APP_USED_DATE3, 0L);
        mLastTimeUsedDate4 = sharedPreferencesManager.readLong(PREF_LAST_TIME_APP_USED_DATE4, 0L);

        if (dayDifference(today, mLastTimeUsedDate1) == 0) {
            return;
        }

        mLastTimeUsedDate4 = mLastTimeUsedDate3;
        mLastTimeUsedDate3 = mLastTimeUsedDate2;
        mLastTimeUsedDate2 = mLastTimeUsedDate1;

        mLastTimeUsedDate1 = today;

        sharedPreferencesManager.writeLong(PREF_LAST_TIME_APP_USED_DATE1, mLastTimeUsedDate1);
        sharedPreferencesManager.writeLong(PREF_LAST_TIME_APP_USED_DATE2, mLastTimeUsedDate2);
        sharedPreferencesManager.writeLong(PREF_LAST_TIME_APP_USED_DATE3, mLastTimeUsedDate3);
        sharedPreferencesManager.writeLong(PREF_LAST_TIME_APP_USED_DATE4, mLastTimeUsedDate4);
    }

    private boolean is4DaysUsedLast7Days() {
        return dayDifference(mLastTimeUsedDate1, mLastTimeUsedDate2) <= LAST_7_DAYS
                && dayDifference(mLastTimeUsedDate1, mLastTimeUsedDate3) <= LAST_7_DAYS
                && dayDifference(mLastTimeUsedDate1, mLastTimeUsedDate4) <= LAST_7_DAYS;
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

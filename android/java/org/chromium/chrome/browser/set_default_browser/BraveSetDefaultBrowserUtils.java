/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.set_default_browser;

import android.app.Activity;
import android.app.role.RoleManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.components.embedder_support.util.UrlConstants;

import java.util.Calendar;
import java.util.Date;

public class BraveSetDefaultBrowserUtils {
    private static final String TAG = "BSDBrowserUtils";

    public static final String BRAVE_DEFAULT_SET_COUNTER = "brave_default_set_counter";
    public static final String BRAVE_DEFAULT_TIMER_DAY = "brave_default_timer_day";
    public static final String BRAVE_DEFAULT_APP_OPEN_COUNTER = "brave_default_app_open_counter";
    public static final String BRAVE_DEFAULT_SHOW_TIME = "brave_default_show_time";

    public static final int DAYS_NONE = -1;
    public static final int DAYS_7 = 7;
    public static final int DAYS_8 = 8;
    public static final int DAYS_15 = 15;

    public static boolean isBottomSheetVisible;

    public static boolean isBraveSetAsDefaultBrowser(Context context) {
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        ResolveInfo resolveInfo =
                context.getPackageManager()
                        .resolveActivity(browserIntent, PackageManager.MATCH_DEFAULT_ONLY);
        if (resolveInfo == null
                || resolveInfo.activityInfo == null
                || resolveInfo.activityInfo.packageName == null) {
            return false;
        }

        if (context.getPackageName().equals(BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME)) {
            return resolveInfo.activityInfo.packageName.equals(
                    BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME);
        } else {
            return resolveInfo.activityInfo.packageName.equals(
                           BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(
                            BraveConstants.BRAVE_BETA_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(
                            BraveConstants.BRAVE_NIGHTLY_PACKAGE_NAME);
        }
    }

    public static boolean isAppSetAsDefaultBrowser(Context context) {
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        ResolveInfo resolveInfo =
                context.getPackageManager()
                        .resolveActivity(browserIntent, PackageManager.MATCH_DEFAULT_ONLY);
        if (resolveInfo == null
                || resolveInfo.activityInfo == null
                || resolveInfo.activityInfo.packageName == null) {
            return false;
        }
        return resolveInfo.activityInfo.packageName.equals(context.getPackageName());
    }

    public static void showBraveSetDefaultBrowserDialog(AppCompatActivity activity) {

        if (!isBottomSheetVisible) {
            isBottomSheetVisible = true;

            try {
                SetDefaultBrowserBottomSheetFragment bottomSheetDialog =
                        new SetDefaultBrowserBottomSheetFragment();

                bottomSheetDialog.show(
                        activity.getSupportFragmentManager(),
                        "SetDefaultBrowserBottomSheetFragment");
            } catch (IllegalStateException e) {
                // That exception could be thrown when Activity is not in the foreground.
                Log.e(TAG, "showBraveSetDefaultBrowserDialog error: " + e.getMessage());
                return;
            }
        }
    }

    public static void openDefaultAppsSettings(Activity activity) {
        Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
        activity.startActivity(intent);
    }

    // New changes
    public static void checkForBraveSetDefaultBrowser(
            int appOpenCount, AppCompatActivity activity) {
        if (appOpenCount == 0 && shouldSetBraveDefaultSetCounter()) {
            setBraveDefaultSetCounter();
            setBraveDefaultShowTimer(DAYS_7);
        }
        if (getBraveDefaultTimerDay() != DAYS_NONE) {
            incrementBraveDefaultAppOpenCounter();
            decideToShowBraveSetDefaultBrowserDialog(activity);
        }
    }

    private static void setBraveDefaultShowTimer(int days) {
        Calendar calender = Calendar.getInstance();
        calender.setTime(new Date());
        calender.add(Calendar.DATE, days);
        setBraveDefaultShowTime(calender.getTimeInMillis());
        setBraveDefaultTimerDay(days);
    }

    /**
     * Checks if the device supports the Default Role Manager API. This API was introduced in
     * Android 10 (API level 29) and allows apps to request to be set as default handlers for
     * specific roles like browser, phone, etc.
     *
     * @return true if the device is running Android 10 or higher, false otherwise
     */
    public static boolean supportsDefaultRoleManager() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q;
    }

    public static void decideToShowBraveSetDefaultBrowserDialog(AppCompatActivity activity) {
        logAppOpenCounter();

        if (shouldShowDefaultBrowserDialog(activity)) {
            handleDefaultBrowserPrompt(activity);
        }
    }

    private static void logAppOpenCounter() {
        Log.e(
                "brave_default",
                "getBraveDefaultAppOpenCounter() : " + getBraveDefaultAppOpenCounter());
    }

    private static boolean shouldShowDefaultBrowserDialog(Activity activity) {
        return !isBraveSetAsDefaultBrowser(activity)
                && getBraveDefaultAppOpenCounter() >= 5
                && System.currentTimeMillis() > getBraveDefaultShowTime();
    }

    private static void handleDefaultBrowserPrompt(AppCompatActivity activity) {
        setDefaultBrowser(activity, false);
        resetBraveDefaultAppOpenCounter();
        updateNextPromptTimer();
    }

    private static void updateNextPromptTimer() {
        int nextDay = calculateNextPromptDay();
        Log.e("brave_default", "nextDay : " + nextDay);
        setBraveDefaultShowTimer(nextDay);
    }

    private static int calculateNextPromptDay() {
        int currentDay = getBraveDefaultTimerDay();
        if (currentDay == DAYS_7) {
            return DAYS_8;
        } else if (currentDay == DAYS_8) {
            return DAYS_15;
        }
        return -1;
    }

    public static void setDefaultBrowser(AppCompatActivity activity, boolean isFromOnboarding) {
        Log.e("brave_default", "setDefaultBrowser 1 : ");
        if (supportsDefaultRoleManager()) {
            Log.e("brave_default", "setDefaultBrowser 2 : ");
            RoleManager roleManager = activity.getSystemService(RoleManager.class);

            if (roleManager.isRoleAvailable(RoleManager.ROLE_BROWSER)) {
                Log.e("brave_default", "setDefaultBrowser 3 : ");
                if (!roleManager.isRoleHeld(RoleManager.ROLE_BROWSER)) {
                    Log.e("brave_default", "setDefaultBrowser 4 : ");
                    activity.startActivityForResult(
                            roleManager.createRequestRoleIntent(RoleManager.ROLE_BROWSER),
                            BraveConstants.DEFAULT_BROWSER_ROLE_REQUEST_CODE);
                }
            } else {
                Log.e("brave_default", "setDefaultBrowser 5 : ");
                openDefaultAppsSettings(activity);
            }
        } else {
            if (!isFromOnboarding) {
                showBraveSetDefaultBrowserDialog(activity);
            } else {
                openDefaultAppsSettings(activity);
            }
        }
    }

    public static boolean shouldSetBraveDefaultSetCounter() {
        return ChromeSharedPreferences.getInstance().readBoolean(BRAVE_DEFAULT_SET_COUNTER, true);
    }

    public static void setBraveDefaultSetCounter() {
        ChromeSharedPreferences.getInstance().writeBoolean(BRAVE_DEFAULT_SET_COUNTER, false);
    }

    public static void incrementBraveDefaultAppOpenCounter() {
        ChromeSharedPreferences.getInstance()
                .writeInt(BRAVE_DEFAULT_APP_OPEN_COUNTER, getBraveDefaultAppOpenCounter() + 1);
    }

    public static int getBraveDefaultAppOpenCounter() {
        return ChromeSharedPreferences.getInstance().readInt(BRAVE_DEFAULT_APP_OPEN_COUNTER, 0);
    }

    public static void resetBraveDefaultAppOpenCounter() {
        ChromeSharedPreferences.getInstance().writeInt(BRAVE_DEFAULT_APP_OPEN_COUNTER, 0);
    }

    public static void setBraveDefaultShowTime(long nextDay) {
        ChromeSharedPreferences.getInstance().writeLong(BRAVE_DEFAULT_SHOW_TIME, nextDay);
    }

    public static long getBraveDefaultShowTime() {
        return ChromeSharedPreferences.getInstance().readLong(BRAVE_DEFAULT_SHOW_TIME, 0);
    }

    public static void setBraveDefaultTimerDay(int nextDays) {
        ChromeSharedPreferences.getInstance().writeInt(BRAVE_DEFAULT_TIMER_DAY, nextDays);
    }

    public static int getBraveDefaultTimerDay() {
        return ChromeSharedPreferences.getInstance().readInt(BRAVE_DEFAULT_TIMER_DAY, DAYS_NONE);
    }
}

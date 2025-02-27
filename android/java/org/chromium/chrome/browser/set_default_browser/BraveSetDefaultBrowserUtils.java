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
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.ui.widget.Toast;

import java.util.Calendar;
import java.util.Date;

public class BraveSetDefaultBrowserUtils {
    private static final String TAG = "BSDBrowserUtils";
    public static final String ANDROID_SETUPWIZARD_PACKAGE_NAME = "com.google.android.setupwizard";
    public static final String ANDROID_PACKAGE_NAME = "android";
    public static final String BRAVE_BLOG_URL = "https://brave.com/privacy-features/";

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

    public static void checkSetDefaultBrowserModal(AppCompatActivity activity) {
        if (!isBraveSetAsDefaultBrowser(activity) && !isBraveDefaultDontAsk()) {
            if (ChromeSharedPreferences.getInstance()
                            .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                    == 5) {
                showBraveSetDefaultBrowserDialog(activity, false);

            } else if (shouldShowBraveWasDefaultDialog()) {
                int braveWasDefaultCount =
                        ChromeSharedPreferences.getInstance()
                                .readInt(BravePreferenceKeys.BRAVE_WAS_DEFAULT_ASK_COUNT);
                ChromeSharedPreferences.getInstance()
                        .writeInt(
                                BravePreferenceKeys.BRAVE_WAS_DEFAULT_ASK_COUNT,
                                braveWasDefaultCount + 1);
                showBraveSetDefaultBrowserDialog(activity, false);
            }

        } else if (isBraveSetAsDefaultBrowser(activity) && !wasBraveDefaultBefore()) {
            setBraveDefaultSuccess();
        }
    }

    public static void showBraveSetDefaultBrowserDialog(
            AppCompatActivity activity, boolean isFromMenu) {
        /* (Albert Wang): Default app settings didn't get added until API 24
         * https://developer.android.com/reference/android/provider/Settings#ACTION_MANAGE_DEFAULT_APPS_SETTINGS
         */
        if (isBraveSetAsDefaultBrowser(activity)) {
            Toast toast = Toast.makeText(
                    activity, R.string.brave_already_set_as_default_browser, Toast.LENGTH_LONG);
            toast.show();
            return;
        }

        if (!isBottomSheetVisible) {
            isBottomSheetVisible = true;

            try {
                SetDefaultBrowserBottomSheetFragment bottomSheetDialog =
                        SetDefaultBrowserBottomSheetFragment.newInstance(isFromMenu);

                bottomSheetDialog.show(activity.getSupportFragmentManager(),
                        "SetDefaultBrowserBottomSheetFragment");
            } catch (IllegalStateException e) {
                // That exception could be thrown when Activity is not in the foreground.
                Log.e(TAG, "showBraveSetDefaultBrowserDialog error: " + e.getMessage());
                return;
            }

            if (!isFromMenu) {
                int braveDefaultModalCount =
                        ChromeSharedPreferences.getInstance()
                                .readInt(BravePreferenceKeys.BRAVE_SET_DEFAULT_BOTTOM_SHEET_COUNT);
                ChromeSharedPreferences.getInstance()
                        .writeInt(
                                BravePreferenceKeys.BRAVE_SET_DEFAULT_BOTTOM_SHEET_COUNT,
                                braveDefaultModalCount + 1);
            }
        }
    }

    private static ResolveInfo getResolveInfo(Activity activity) {
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));

        return activity.getPackageManager()
                .resolveActivity(browserIntent, PackageManager.MATCH_DEFAULT_ONLY);
    }

    public static void openDefaultAppsSettings(Activity activity) {
        Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
        activity.startActivity(intent);
    }

    private static void openBraveBlog(Activity activity) {
        LayoutInflater inflater = activity.getLayoutInflater();
        View layout = inflater.inflate(R.layout.brave_set_default_browser_dialog,
                (ViewGroup) activity.findViewById(R.id.brave_set_default_browser_toast_container));

        Toast toast = new Toast(activity, layout);
        toast.setDuration(Toast.LENGTH_LONG);
        toast.setGravity(Gravity.TOP, 0, 40);
        toast.show();

        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(BRAVE_BLOG_URL));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(intent);
    }

    private static boolean wasBraveDefaultBefore() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_IS_DEFAULT, false);
    }

    private static boolean shouldShowBraveWasDefaultDialog() {
        int braveWasDefaultCount =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePreferenceKeys.BRAVE_WAS_DEFAULT_ASK_COUNT);
        return braveWasDefaultCount < 2 && wasBraveDefaultBefore();
    }

    public static void setBraveDefaultSuccess() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_IS_DEFAULT, true);
    }

    public static void setBraveDefaultDontAsk() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_DEFAULT_DONT_ASK, true);
    }

    public static boolean isBraveDefaultDontAsk() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_DEFAULT_DONT_ASK, false);
    }

    // New changes
    public static void checkForBraveSetDefaultBrowser(int appOpenCount, Activity activity) {
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

    public static boolean supportsDefaultRoleManager() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q;
    }

    public static void decideToShowBraveSetDefaultBrowserDialog(Activity activity) {
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

    private static void handleDefaultBrowserPrompt(Activity activity) {
        setDefaultBrowser(activity);
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

    public static void setDefaultBrowser(Activity activity) {
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
            Log.e("brave_default", "setDefaultBrowser 6 : ");
            ResolveInfo resolveInfo = getResolveInfo(activity);
            if (resolveInfo.activityInfo.packageName.equals(ANDROID_SETUPWIZARD_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(ANDROID_PACKAGE_NAME)) {
                Log.e("brave_default", "setDefaultBrowser 7 : ");
                openBraveBlog(activity);
            } else {
                Log.e("brave_default", "setDefaultBrowser 8 : ");
                Toast toast =
                        Toast.makeText(
                                activity,
                                R.string.brave_default_browser_go_to_settings,
                                Toast.LENGTH_LONG);
                toast.show();
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

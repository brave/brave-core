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
import android.provider.Settings;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.components.embedder_support.util.UrlConstants;

import java.util.Calendar;
import java.util.Date;

/**
 * Utility class for managing Brave browser's default browser status. Provides functionality to: -
 * Check if Brave is set as the default browser - Set Brave as the default browser - Track default
 * browser prompting state - Handle default browser settings interactions
 */
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

    /**
     * Checks if any Brave browser variant (Production, Beta, or Nightly) is set as the default
     * browser. For production builds, only checks if production Brave is default. For other builds
     * (Beta/Nightly), checks if any Brave variant is default.
     *
     * @param context The application context
     * @return true if appropriate Brave variant is set as default browser, false otherwise
     */
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

    /**
     * Checks if the current app is set as the default browser. This differs from
     * isBraveSetAsDefaultBrowser() by only checking if the current package is the default, rather
     * than checking for specific Brave browser variants.
     *
     * @param context The application context
     * @return true if this app is set as default browser, false otherwise
     */
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

    /**
     * Shows a bottom sheet dialog prompting the user to set Brave as their default browser. The
     * dialog will only be shown if no other bottom sheet is currently visible.
     *
     * @param activity The current AppCompatActivity instance needed to show the dialog
     * @throws IllegalStateException if the activity is not in foreground when showing dialog
     */
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

    /**
     * Opens the system settings page for managing default apps. This allows users to change their
     * default browser and other app defaults.
     *
     * @param activity The activity context needed to start the settings activity
     */
    public static void openDefaultAppsSettings(Activity activity) {
        Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
        activity.startActivity(intent);
    }

    /**
     * Checks and manages the default browser setting prompt flow. This method handles the logic for
     * when to show the default browser prompt to users.
     *
     * @param appOpenCount Number of times the app has been opened, used to determine first launch
     * @param activity The current activity context needed for showing dialogs
     */
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

    /**
     * Sets a timer for when to show the "Set Brave as Default Browser" prompt again.
     *
     * @param days Number of days to wait before showing the prompt again
     */
    private static void setBraveDefaultShowTimer(int days) {
        Calendar calendar = Calendar.getInstance();
        calendar.setTime(new Date());
        // Add specified number of days
        calendar.add(Calendar.DATE, days);
        // Store the future timestamp when we should show the prompt
        setBraveDefaultShowTime(calendar.getTimeInMillis());
        // Store the number of days we're waiting
        setBraveDefaultTimerDay(days);
    }

    public static void decideToShowBraveSetDefaultBrowserDialog(AppCompatActivity activity) {
        if (shouldShowDefaultBrowserDialog(activity)) {
            handleDefaultBrowserPrompt(activity);
        }
    }

    /**
     * Determines whether the "Set Brave as Default Browser" dialog should be shown to the user.
     *
     * @param activity The current Activity context
     * @return true if all conditions are met to show the dialog: - Brave is not currently set as
     *     the default browser - The app has been opened at least 5 times since the last prompt -
     *     The configured wait time has elapsed since the last prompt
     */
    private static boolean shouldShowDefaultBrowserDialog(Activity activity) {
        return !isBraveSetAsDefaultBrowser(activity)
                && getBraveDefaultAppOpenCounter() >= 5
                && System.currentTimeMillis() > getBraveDefaultShowTime();
    }

    private static void handleDefaultBrowserPrompt(AppCompatActivity activity) {
        setDefaultBrowser(activity);
        resetBraveDefaultAppOpenCounter();
        updateNextPromptTimer();
    }

    private static void updateNextPromptTimer() {
        int nextDay = calculateNextPromptDay();
        setBraveDefaultShowTimer(nextDay);
    }

    /**
     * Calculates the next day to show the default browser prompt based on the current timer day.
     * The prompt timing follows a progression from 7 days to 8 days to 15 days.
     *
     * @return The next day to show the prompt: - Returns 8 days if current timer is 7 days -
     *     Returns 15 days if current timer is 8 days - Returns -1 for any other current timer value
     */
    private static int calculateNextPromptDay() {
        int currentDay = getBraveDefaultTimerDay();
        if (currentDay == DAYS_7) {
            return DAYS_8;
        } else if (currentDay == DAYS_8) {
            return DAYS_15;
        }
        return -1;
    }

    /**
     * Attempts to set Brave as the default browser using the appropriate system mechanism.
     *
     * <p>On Android 10+ devices that support the RoleManager API, this will: - Request the browser
     * role if available and not already held - Open system default apps settings if role is
     * unavailable
     *
     * @param activity The current activity context
     */
    public static void setDefaultBrowser(AppCompatActivity activity) {
        RoleManager roleManager = activity.getSystemService(RoleManager.class);

        if (roleManager.isRoleAvailable(RoleManager.ROLE_BROWSER)) {
            if (!roleManager.isRoleHeld(RoleManager.ROLE_BROWSER)) {
                activity.startActivityForResult(
                        roleManager.createRequestRoleIntent(RoleManager.ROLE_BROWSER),
                        BraveConstants.DEFAULT_BROWSER_ROLE_REQUEST_CODE);
            }
        } else {
            openDefaultAppsSettings(activity);
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

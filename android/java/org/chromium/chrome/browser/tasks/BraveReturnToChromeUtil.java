/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks;

import android.content.Intent;
import android.os.Bundle;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ChromeInactivityTracker;
import org.chromium.chrome.browser.ntp.BraveFreshNtpHelper;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabCreator;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

@NullMarked
public final class BraveReturnToChromeUtil {

    // Inactivity threshold for variant B: 1 hour in milliseconds
    private static final long INACTIVITY_THRESHOLD_MS_VARIANT_B = 60 * 60 * 1000L; // 1 hour
    // Inactivity threshold for variant C: 2 hours in milliseconds
    private static final long INACTIVITY_THRESHOLD_MS_VARIANT_C = 2 * 60 * 60 * 1000L; // 2 hours

    /** Returns whether should show a NTP as the home surface at startup. */
    public static boolean shouldShowNtpAsHomeSurfaceAtStartup(
            Intent intent, Bundle bundle, ChromeInactivityTracker inactivityTracker) {
        // When feature is disabled, use Brave's default behavior
        if (!BraveFreshNtpHelper.isEnabled()) {
            return false;
        }

        String variant = BraveFreshNtpHelper.getVariant();
        switch (variant) {
            case "A":
                // Variant A: Brave's default behavior (no NTP at startup)
                return false;
            case "B":
                // Variant B: Show NTP if app has been backgrounded for ≥ 1 hour
                // and OPTION_NEW_TAB_AFTER_INACTIVITY is selected
                return shouldShowNtpForInactivityVariant(
                        inactivityTracker, INACTIVITY_THRESHOLD_MS_VARIANT_B);
            case "C":
                // Variant C: Show NTP if app has been backgrounded for ≥ 2 hours
                // and OPTION_NEW_TAB_AFTER_INACTIVITY is selected
                return shouldShowNtpForInactivityVariant(
                        inactivityTracker, INACTIVITY_THRESHOLD_MS_VARIANT_C);
            default:
                // All other variants: fallback to upstream behavior
                // return ReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                //        intent, bundle, inactivityTracker);
        }
        return false;
    }

    /**
     * Checks if NTP should be shown based on inactivity duration and preference.
     *
     * @param inactivityTracker The inactivity tracker to check background time
     * @param inactivityThresholdMs The inactivity threshold in milliseconds
     * @return true if NTP should be shown, false otherwise
     */
    private static boolean shouldShowNtpForInactivityVariant(
            ChromeInactivityTracker inactivityTracker, long inactivityThresholdMs) {
        // Check opening screen option first
        int openingScreenOption =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePreferenceKeys.BRAVE_NEW_TAB_PAGE_OPENING_SCREEN, 1);

        // If user has selected "New Tab" option, return true without checking timer
        if (openingScreenOption == BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB) {
            // Set a flag to indicate NTP was shown, so snackbar can be displayed
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, true);
            return true;
        }

        // If OPTION_NEW_TAB_AFTER_INACTIVITY is not selected, return false
        if (openingScreenOption
                        != BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB_AFTER_INACTIVITY
                || inactivityTracker == null) {
            return false;
        }

        // Check if app has been backgrounded for ≥ threshold
        long timeSinceLastBackgroundedMs = inactivityTracker.getTimeSinceLastBackgroundedMs();
        boolean shouldShow = timeSinceLastBackgroundedMs >= inactivityThresholdMs;

        // Set a flag to indicate NTP was shown after returning from background
        // This flag will be checked in BraveNewTabPageLayout to show the snackbar
        if (shouldShow) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, true);
        }

        return shouldShow;
    }

    /**
     * Creates a new tab and shows the home surface UI. This method captures the last active tab URL
     * before creating the NTP, which can be used later to restore the correct tab.
     */
    public static @Nullable Tab createNewTabAndShowHomeSurfaceUi(
            TabCreator tabCreator,
            HomeSurfaceTracker homeSurfaceTracker,
            @Nullable TabModelSelector tabModelSelector,
            @Nullable String lastActiveTabUrl,
            @Nullable Tab lastActiveTab) {
        // Store the last active tab URL if provided. This is used to restore the correct tab
        // when the app is returned from background in BraveRecentTabsSnackbarHelper.
        if (lastActiveTabUrl != null && !lastActiveTabUrl.isEmpty()) {
            ChromeSharedPreferences.getInstance()
                    .writeString(BravePreferenceKeys.BRAVE_LAST_ACTIVE_TAB_URL, lastActiveTabUrl);
        }

        // Call the upstream method to create the NTP
        return ReturnToChromeUtil.createNewTabAndShowHomeSurfaceUi(
                tabCreator, homeSurfaceTracker, tabModelSelector, lastActiveTabUrl, lastActiveTab);
    }
}

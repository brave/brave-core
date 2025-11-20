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
import org.chromium.url.GURL;

@NullMarked
public final class BraveReturnToChromeUtil {

    // Inactivity threshold for variants B and D: 1 hour in milliseconds
    private static final long INACTIVITY_THRESHOLD_MS_VARIANT_B_D = 60 * 60 * 1000L; // 1 hour
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
        boolean shouldShow = false;
        switch (variant) {
            case "A":
                // Variant A: Brave's default behavior (no NTP at startup)
                shouldShow = false;
                break;
            case "B":
            case "D":
                // Variants B and D: Show NTP if app has been backgrounded for ≥ 1 hour
                // and OPTION_NEW_TAB_AFTER_INACTIVITY is selected
                shouldShow =
                        shouldShowNtpForInactivityVariant(
                                inactivityTracker, INACTIVITY_THRESHOLD_MS_VARIANT_B_D);
                break;
            case "C":
                // Variant C: Show NTP if app has been backgrounded for ≥ 2 hours
                // and OPTION_NEW_TAB_AFTER_INACTIVITY is selected
                shouldShow =
                        shouldShowNtpForInactivityVariant(
                                inactivityTracker, INACTIVITY_THRESHOLD_MS_VARIANT_C);
                break;
        }

        // Set flag only when shouldShowNtpAsHomeSurfaceAtStartup returns true
        // This ensures maybeShowRecentTabsDialog only executes when this method was called
        if (shouldShow) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, true);
        }

        return shouldShow;
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
            boolean foregroundSessionEndTriggered =
                    ChromeSharedPreferences.getInstance()
                            .readBoolean(
                                    BravePreferenceKeys.BRAVE_FOREGROUND_SESSION_ENDS_TRIGGERED,
                                    false);
            // If foreground session ends has been triggered, return true and reset the flag
            if (foregroundSessionEndTriggered) {
                ChromeSharedPreferences.getInstance()
                        .writeBoolean(
                                BravePreferenceKeys.BRAVE_FOREGROUND_SESSION_ENDS_TRIGGERED, false);
                return true;
            }
            // If foreground session ends has not been triggered, return false.
            // It could be because of Settings Activity or other activities.
            return false;
        }

        // If OPTION_NEW_TAB_AFTER_INACTIVITY is not selected, return false
        if (openingScreenOption
                        != BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB_AFTER_INACTIVITY
                || inactivityTracker == null) {
            return false;
        }
        // Check if app has been backgrounded for ≥ threshold
        long timeSinceLastBackgroundedMs = inactivityTracker.getTimeSinceLastBackgroundedMs();

        return timeSinceLastBackgroundedMs >= inactivityThresholdMs;
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
        String urlToStore = lastActiveTabUrl;
        // If URL is not provided, try to get it from lastActiveTab
        if ((urlToStore == null || urlToStore.isEmpty()) && lastActiveTab != null) {
            GURL url = lastActiveTab.getUrl();
            if (url != null && !url.isEmpty()) {
                urlToStore = url.getSpec();
            }
        }

        if (urlToStore != null && !urlToStore.isEmpty()) {
            ChromeSharedPreferences.getInstance()
                    .writeString(BravePreferenceKeys.BRAVE_LAST_ACTIVE_TAB_URL, urlToStore);
        }

        // Call the upstream method to create the NTP
        return ReturnToChromeUtil.createNewTabAndShowHomeSurfaceUi(
                tabCreator, homeSurfaceTracker, tabModelSelector, lastActiveTabUrl, lastActiveTab);
    }
}

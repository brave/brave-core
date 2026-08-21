/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.AppearancePreferences;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

public class NtpUtil {
    public static final int TOP_SITES_MODE_SHORTCUTS = 0;
    public static final int TOP_SITES_MODE_FREQUENT = 1;

    private static final String PREF_NTP_CUSTOM_LINKS_VISIBLE = "ntp.custom_links_visible";

    public static boolean shouldDisplayTopSites() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BackgroundImagesPreferences.PREF_SHOW_TOP_SITES, true);
    }

    public static void setDisplayTopSites(boolean shouldDisplayTopSites) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(
                        BackgroundImagesPreferences.PREF_SHOW_TOP_SITES, shouldDisplayTopSites);
    }

    public static boolean shouldDisplayBraveStats() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS, true);
    }

    public static void setDisplayBraveStats(boolean shouldDisplayBraveStats) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(
                        BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS, shouldDisplayBraveStats);
    }

    public static boolean shouldShowRewardsIcon() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(AppearancePreferences.PREF_SHOW_BRAVE_REWARDS_ICON, true);
    }

    public static int getTopSitesDisplayMode() {
        if (ProfileManager.isInitialized()) {
            PrefService prefs = UserPrefs.get(ProfileManager.getLastUsedRegularProfile());
            boolean customLinksVisible = prefs.getBoolean(PREF_NTP_CUSTOM_LINKS_VISIBLE);
            return customLinksVisible ? TOP_SITES_MODE_SHORTCUTS : TOP_SITES_MODE_FREQUENT;
        }
        return ChromeSharedPreferences.getInstance()
                .readInt(
                        BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE,
                        TOP_SITES_MODE_SHORTCUTS);
    }

    public static void setTopSitesDisplayMode(int mode) {
        if (ProfileManager.isInitialized()) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(PREF_NTP_CUSTOM_LINKS_VISIBLE, mode == TOP_SITES_MODE_SHORTCUTS);
        }
        // Always write the Android SharedPref to fire the local listener.
        ChromeSharedPreferences.getInstance()
                .writeInt(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE, mode);
    }
}

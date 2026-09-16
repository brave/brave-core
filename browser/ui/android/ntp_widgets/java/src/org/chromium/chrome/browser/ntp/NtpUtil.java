/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;

@NullMarked
public class NtpUtil {
    public static final int TOP_SITES_MODE_SHORTCUTS = 0;
    public static final int TOP_SITES_MODE_FREQUENT = 1;

    private static final String PREF_NTP_CUSTOM_LINKS_VISIBLE = "ntp.custom_links_visible";

    // Mirrors of the Settings-screen pref keys of the same name, duplicated (rather than
    // imported) so this class does not depend on the settings package, which lets it live in its
    // own build target. Keep in sync with BackgroundImagesPreferences.PREF_SHOW_TOP_SITES /
    // PREF_SHOW_BRAVE_STATS and AppearancePreferences.PREF_SHOW_BRAVE_REWARDS_ICON.
    private static final String PREF_SHOW_TOP_SITES = "show_top_sites";
    private static final String PREF_SHOW_BRAVE_STATS = "show_brave_stats";
    private static final String PREF_SHOW_BRAVE_REWARDS_ICON = "show_brave_rewards_icon";

    public static boolean shouldDisplayTopSites() {
        return ChromeSharedPreferences.getInstance().readBoolean(PREF_SHOW_TOP_SITES, true);
    }

    public static void setDisplayTopSites(boolean shouldDisplayTopSites) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(PREF_SHOW_TOP_SITES, shouldDisplayTopSites);
    }

    public static boolean shouldDisplayBraveStats() {
        return ChromeSharedPreferences.getInstance().readBoolean(PREF_SHOW_BRAVE_STATS, true);
    }

    public static void setDisplayBraveStats(boolean shouldDisplayBraveStats) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(PREF_SHOW_BRAVE_STATS, shouldDisplayBraveStats);
    }

    public static boolean shouldShowRewardsIcon() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(PREF_SHOW_BRAVE_REWARDS_ICON, true);
    }

    public static int getTopSitesDisplayMode() {
        return ChromeSharedPreferences.getInstance()
                .readInt(
                        BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE,
                        TOP_SITES_MODE_SHORTCUTS);
    }

    public static void setTopSitesDisplayMode(int mode) {
        ChromeSharedPreferences.getInstance()
                .writeInt(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE, mode);
    }

    /**
     * Reads the Desktop-shared {@code ntp.custom_links_visible} profile pref. Only {@link
     * org.chromium.chrome.browser.suggestions.mostvisited.BraveMostVisitedSites} should call this,
     * since it is the sole owner of a legitimately-scoped {@link Profile} reference in this
     * feature; everywhere else should use {@link #getTopSitesDisplayMode} instead.
     */
    public static int getProfileTopSitesDisplayMode(Profile profile) {
        boolean customLinksVisible =
                UserPrefs.get(profile).getBoolean(PREF_NTP_CUSTOM_LINKS_VISIBLE);
        return customLinksVisible ? TOP_SITES_MODE_SHORTCUTS : TOP_SITES_MODE_FREQUENT;
    }

    /** Writes {@code mode} to the Desktop-shared {@code ntp.custom_links_visible} profile pref. */
    public static void setProfileTopSitesDisplayMode(Profile profile, int mode) {
        UserPrefs.get(profile)
                .setBoolean(PREF_NTP_CUSTOM_LINKS_VISIBLE, mode == TOP_SITES_MODE_SHORTCUTS);
    }
}

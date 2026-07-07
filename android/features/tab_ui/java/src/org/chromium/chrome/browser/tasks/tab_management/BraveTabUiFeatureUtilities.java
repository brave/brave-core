/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;

@NullMarked
public class BraveTabUiFeatureUtilities {
    public static boolean isTabGroupsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);
    }

    public static void setTabGroupsEnabled(Profile profile, boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
        // Reconcile the effective "auto open synced tab groups" native pref here rather than in the
        // settings UI, so that any caller that toggles the master switch keeps runtime consumers in
        // sync without depending on the settings screen being opened.
        applyAutoOpenSyncedTabGroupsEffectivePref(profile);
    }

    public static boolean isBraveTabGroupsEnabled() {
        return isTabGroupsEnabled() && isOpenLinksInCurrentTabGroupEnabled();
    }

    /**
     * Returns the user's intended "auto open synced tab groups" choice, defaulting to the current
     * native pref value so an existing user's choice is preserved on first run of this logic.
     */
    public static boolean getAutoOpenSyncedTabGroupsUserChoice(Profile profile) {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_AUTO_OPEN_SYNCED_TAB_GROUPS_USER_CHOICE,
                        UserPrefs.get(profile).getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS));
    }

    /**
     * Stores the user's intended "auto open synced tab groups" choice and re-applies the effective
     * native pref.
     */
    public static void setAutoOpenSyncedTabGroupsUserChoice(Profile profile, boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(
                        BravePreferenceKeys.BRAVE_AUTO_OPEN_SYNCED_TAB_GROUPS_USER_CHOICE, enabled);
        applyAutoOpenSyncedTabGroupsEffectivePref(profile);
    }

    /**
     * Keeps the native {@link Pref#AUTO_OPEN_SYNCED_TAB_GROUPS} value equal to the master "Enable
     * tab groups" switch AND the user's intended choice, so disabling tab groups also stops
     * auto-opening synced tab groups. The user's choice is persisted so it can be restored when tab
     * groups are re-enabled.
     */
    public static void applyAutoOpenSyncedTabGroupsEffectivePref(Profile profile) {
        boolean userChoice = getAutoOpenSyncedTabGroupsUserChoice(profile);
        // Persist the resolved choice so it isn't lost once the native pref is forced off.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(
                        BravePreferenceKeys.BRAVE_AUTO_OPEN_SYNCED_TAB_GROUPS_USER_CHOICE,
                        userChoice);
        UserPrefs.get(profile)
                .setBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS, isTabGroupsEnabled() && userChoice);
    }

    public static boolean isOpenLinksInCurrentTabGroupEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED,
                        ChromeSharedPreferences.getInstance()
                                .readBoolean(
                                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE,
                                        true));
    }

    public static void setOpenLinksInCurrentTabGroupEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, enabled);
    }

    public static boolean isTabGroupsBarPreferenceEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, true);
    }

    public static void setTabGroupsBarEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_BAR_ENABLED, enabled);
    }

    public static boolean isBraveAndroidTabGroupsSettingsFeatureEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS);
    }
}

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.ContextUtils;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridge;
import org.chromium.components.content_settings.ContentSetting;
import org.chromium.components.content_settings.ContentSettingsType;

@NullMarked
public class BraveHelper {
    public static final String SHARED_PREF_DISPLAYED_INFOBAR_PROMO =
            "displayed_data_reduction_infobar_promo";
    private static final String OBSOLETE_PREF_ADS_SWITCH = "ads_switch";

    public BraveHelper() {}

    public static void disableFREDRP() {
        // Disables data reduction promo dialog
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(SHARED_PREF_DISPLAYED_INFOBAR_PROMO, true)
                .apply();
    }

    public static void maybeMigrateSettings() {
        // False is the default value, so we want to migrate it only when it's true.
        if (BravePrefServiceBridge.getInstance().getDesktopModeEnabled()) {
            Profile profile = ProfileManager.getLastUsedRegularProfile();
            WebsitePreferenceBridge.setDefaultContentSetting(
                    profile, ContentSettingsType.REQUEST_DESKTOP_SITE, ContentSetting.ALLOW);
            // Reset old flag to default value, so we don't migrate it anymore.
            BravePrefServiceBridge.getInstance().setDesktopModeEnabled(false);
        }

        // The "Show notification ads when Brave is not in use" setting was removed. Clear any
        // stored value so all users fall back to the default (disabled).
        SharedPreferencesManager prefs = ChromeSharedPreferences.getInstance();
        if (prefs.contains(OBSOLETE_PREF_ADS_SWITCH)) {
            prefs.removeKey(OBSOLETE_PREF_ADS_SWITCH);
        }
    }
}

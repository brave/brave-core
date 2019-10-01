/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

public class BraveSiteSettingsPreferences extends SiteSettingsPreferences {
    private static final String DESKTOP_MODE_CATEGORY_KEY = "desktop_mode_category";
    private static final String PLAY_YT_VIDEO_IN_BROWSER_CATEGORY_KEY = "play_yt_video_in_browser_category";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        PreferenceUtils.addPreferencesFromResource(this, R.xml.brave_site_settings_preferences);
        configureBravePreferences();
        updateBravePreferenceStates();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateBravePreferenceStates();
    }

    private void configureBravePreferences() {
        Preference play_yt_video_pref = findPreference(PLAY_YT_VIDEO_IN_BROWSER_CATEGORY_KEY);
        Preference desktop_mode_pref = findPreference(DESKTOP_MODE_CATEGORY_KEY);
        if (mMediaSubMenu) {
            getPreferenceScreen().removePreference(desktop_mode_pref);
        } else {
            getPreferenceScreen().removePreference(play_yt_video_pref);
        }
    }

    private void updateBravePreferenceStates() {
        if (mMediaSubMenu) {
            Preference p = findPreference(PLAY_YT_VIDEO_IN_BROWSER_CATEGORY_KEY);
            boolean enabled = ContextUtils.getAppSharedPreferences().getBoolean(
                PlayYTVideoInBrowserPreferences.PLAY_YT_VIDEO_IN_BROWSER_KEY, true);
            p.setSummary(enabled ? R.string.settings_play_yt_video_in_browser_enabled_summary
                                 : R.string.settings_play_yt_video_in_browser_off);
        } else {
            Preference p = findPreference(DESKTOP_MODE_CATEGORY_KEY);
            boolean enabled = ContextUtils.getAppSharedPreferences().getBoolean(
                 DesktopModePreferences.DESKTOP_MODE_KEY, false);
            p.setSummary(enabled ? R.string.settings_desktop_mode_enabled_summary
                                 : R.string.settings_desktop_mode_disabled_summary);
        }
    }
}

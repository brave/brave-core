/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

public class BraveSiteSettingsPreferencesBase extends PreferenceFragmentCompat {
    private static final String DESKTOP_MODE_CATEGORY_KEY = "desktop_mode_category";
    private static final String PLAY_YT_VIDEO_IN_BROWSER_CATEGORY_KEY = "play_yt_video_in_browser_category";

    private static final String MEDIA_KEY = "media";
    // Whether this class is handling showing the Media sub-menu (and not the main menu).
    private boolean mMediaSubMenu;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (SiteSettingsPreferences::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        PreferenceUtils.addPreferencesFromResource(this, R.xml.brave_site_settings_preferences);
        if (getArguments() != null) {
            String category =
                    getArguments().getString(SingleCategoryPreferences.EXTRA_CATEGORY, "");
            mMediaSubMenu =  MEDIA_KEY.equals(category);
        }
        configureBravePreferences();
        updateBravePreferenceStates();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

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

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.os.Bundle;
import android.support.v7.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.SettingsUtils;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;

import java.util.HashMap;

public class BraveSiteSettingsPreferencesBase extends SiteSettingsPreferenceFragment {
    private static final String DESKTOP_MODE_CATEGORY_KEY = "desktop_mode_category";
    private static final String PLAY_YT_VIDEO_IN_BROWSER_CATEGORY_KEY = "play_yt_video_in_browser_category";
    private static final String ADS_KEY = "ads";
    private static final String BACKGROUND_SYNC_KEY = "background_sync";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (SiteSettingsPreferences::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_site_settings_preferences);
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

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public Preference findPreference(CharSequence key) {
        Preference result = super.findPreference(key);
        if (result == null) {
            result = mRemovedPreferences.get(key);
        }
        return result;
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }

    private void configureBravePreferences() {
        removePreferenceIfPresent(ADS_KEY);
        removePreferenceIfPresent(BACKGROUND_SYNC_KEY);
    }

    private void updateBravePreferenceStates() {
        {
            Preference p = findPreference(PLAY_YT_VIDEO_IN_BROWSER_CATEGORY_KEY);
            boolean enabled = ContextUtils.getAppSharedPreferences().getBoolean(
                PlayYTVideoInBrowserPreferences.PLAY_YT_VIDEO_IN_BROWSER_KEY, true);
            p.setSummary(enabled ? R.string.text_enabled : R.string.text_disabled);
        }
        {
            Preference p = findPreference(DESKTOP_MODE_CATEGORY_KEY);
            boolean enabled = ContextUtils.getAppSharedPreferences().getBoolean(
                 DesktopModePreferences.DESKTOP_MODE_KEY, false);
            p.setSummary(enabled ? R.string.settings_desktop_mode_enabled_summary
                                 : R.string.settings_desktop_mode_disabled_summary);
        }
    }
}

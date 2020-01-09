/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.website;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.settings.ChromeSwitchPreference;
import org.chromium.chrome.browser.settings.SettingsUtils;

public class PlayYTVideoInBrowserPreferences
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    public static final String PLAY_YT_VIDEO_IN_BROWSER_KEY = "play_yt_video_in_browser";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.settings_play_yt_video_in_browser_title);
        SettingsUtils.addPreferencesFromResource(this, R.xml.play_yt_video_in_browser_preferences);

        ChromeSwitchPreference pref =
                (ChromeSwitchPreference) findPreference(PLAY_YT_VIDEO_IN_BROWSER_KEY);
        // Initially enabled.
        pref.setChecked(
            BravePrefServiceBridge.getInstance().getPlayYTVideoInBrowserEnabled());
        pref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        BravePrefServiceBridge.getInstance().setPlayYTVideoInBrowserEnabled((boolean) newValue);
        return true;
    }
}

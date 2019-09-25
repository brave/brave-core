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
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreferenceCompat;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("chrome::android")
public class PlayYTVideoInBrowserPreferences
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    public static final String PLAY_YT_VIDEO_IN_BROWSER_KEY = "play_yt_video_in_browser";
    private Profile mProfile;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.settings_play_yt_video_in_browser_title);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.play_yt_video_in_browser_preferences);

        ChromeSwitchPreferenceCompat pref =
                (ChromeSwitchPreferenceCompat) findPreference(PLAY_YT_VIDEO_IN_BROWSER_KEY);
        // Initially enabled.
        pref.setChecked(
            PlayYTVideoInBrowserPreferencesJni.get().getPlayYTVideoInBrowserEnabled(getProfile()));
        pref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        PlayYTVideoInBrowserPreferencesJni.get().setPlayYTVideoInBrowserEnabled((boolean) newValue, getProfile());
        return true;
    }

    private Profile getProfile() {
        if (mProfile == null)
            mProfile = Profile.getLastUsedProfile();
        return mProfile;
    }

    @NativeMethods
    interface Natives {
        void setPlayYTVideoInBrowserEnabled(boolean enabled, Profile profile);
        boolean getPlayYTVideoInBrowserEnabled(Profile profile);
    }
}

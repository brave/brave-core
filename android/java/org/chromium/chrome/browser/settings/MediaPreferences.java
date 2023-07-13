/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureUtil;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

/* Class for Media section of main preferences */
public class MediaPreferences
        extends BravePreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String PREF_ENABLE_WIDEVINE = "enable_widevine";
    public static final String PREF_BACKGROUND_VIDEO_PLAYBACK = "background_video_playback";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_media);
        SettingsUtils.addPreferencesFromResource(this, R.xml.media_preferences);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        ChromeSwitchPreference enableWidevinePref =
                (ChromeSwitchPreference) findPreference(PREF_ENABLE_WIDEVINE);
        if (enableWidevinePref != null) {
            enableWidevinePref.setChecked(
                    BraveLocalState.get().getBoolean(BravePref.WIDEVINE_OPTED_IN));
            enableWidevinePref.setOnPreferenceChangeListener(this);
        }

        ChromeSwitchPreference backgroundVideoPlaybackPref =
                (ChromeSwitchPreference) findPreference(PREF_BACKGROUND_VIDEO_PLAYBACK);
        if (backgroundVideoPlaybackPref != null) {
            backgroundVideoPlaybackPref.setOnPreferenceChangeListener(this);
            boolean enabled =
                    ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_BACKGROUND_VIDEO_PLAYBACK)
                    || BravePrefServiceBridge.getInstance().getBackgroundVideoPlaybackEnabled();
            backgroundVideoPlaybackPref.setChecked(enabled);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        boolean shouldRelaunch = false;
        if (PREF_ENABLE_WIDEVINE.equals(key)) {
            ChromeSwitchPreference enableWidevinePref =
                    (ChromeSwitchPreference) findPreference(PREF_ENABLE_WIDEVINE);
            BraveLocalState.get().setBoolean(BravePref.WIDEVINE_OPTED_IN,
                    !BraveLocalState.get().getBoolean(BravePref.WIDEVINE_OPTED_IN));
            shouldRelaunch = true;
        } else if (PREF_BACKGROUND_VIDEO_PLAYBACK.equals(key)) {
            BraveFeatureUtil.enableFeature(
                    BraveFeatureList.BRAVE_BACKGROUND_VIDEO_PLAYBACK_INTERNAL, (boolean) newValue,
                    false);
            shouldRelaunch = true;
        }

        if (shouldRelaunch) {
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }

        return true;
    }
}

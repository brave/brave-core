/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveMediaYTQualityPreferences extends BravePreferenceFragment
        implements BraveMediaRadioButtonYTQualityPreference.RadioButtonsDelegate {
    static final String PREF_YT_VIDEO_QUALITY = "yt_video_quality";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.quality_settings_title);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_media_yt_quality_preferences);

        BraveMediaRadioButtonYTQualityPreference mRadioButtons =
                (BraveMediaRadioButtonYTQualityPreference) findPreference(PREF_YT_VIDEO_QUALITY);

        @YTVideoQuality
        int defaultQuality =
                UserPrefs.get(getProfile()).getInteger(BravePref.YT_VIDEO_QUALITY_PREF);

        mRadioButtons.initialize(this, defaultQuality);
    }

    @Override
    public void setDefaultQuality(@YTVideoQuality int defaultQuality) {
        UserPrefs.get(getProfile()).setInteger(BravePref.YT_VIDEO_QUALITY_PREF, defaultQuality);
        BraveRelaunchUtils.askForRelaunch(getActivity());
    }
}

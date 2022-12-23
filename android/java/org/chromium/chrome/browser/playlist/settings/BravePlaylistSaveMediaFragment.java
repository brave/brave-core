/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BravePlaylistSaveMediaFragment extends PreferenceFragmentCompat {
    private static final String PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE = "auto_save_media_for_offline";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.auto_save_media_for_offline);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_playlist_save_media_preference);

        RadioButtonGroupPlaylistAutoSavePreference radioButtonGroupPlaylistAutoSavePreference =
                (RadioButtonGroupPlaylistAutoSavePreference) findPreference(
                        PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE);

        radioButtonGroupPlaylistAutoSavePreference.initialize(
                SharedPreferencesManager.getInstance().readInt(
                        PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE, 0));

        radioButtonGroupPlaylistAutoSavePreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    int method = (int) newValue;
                    SharedPreferencesManager.getInstance().readInt(
                            PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE, method);
                    return true;
                });
    }
}

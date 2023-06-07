/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistService;

public class BravePlaylistSaveMediaFragment
        extends PreferenceFragmentCompat implements ConnectionErrorHandler {
    private PlaylistService mPlaylistService;

    @Override
    public void onConnectionError(MojoException e) {
        mPlaylistService = null;
        initPlaylistService();
    }

    private void initPlaylistService() {
        if (mPlaylistService != null) {
            return;
        }

        mPlaylistService = PlaylistServiceFactoryAndroid.getInstance().getPlaylistService(this);
    }

    @Override
    public void onDestroy() {
        if (mPlaylistService != null) {
            mPlaylistService.close();
        }
        super.onDestroy();
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.auto_save_media_for_offline);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_playlist_save_media_preference);

        initPlaylistService();

        RadioButtonGroupPlaylistAutoSavePreference radioButtonGroupPlaylistAutoSavePreference =
                (RadioButtonGroupPlaylistAutoSavePreference) findPreference(
                        BravePlaylistPreferences.PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE);

        radioButtonGroupPlaylistAutoSavePreference.initialize(
                SharedPreferencesManager.getInstance().readInt(
                        BravePlaylistPreferences.PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE, 0));

        radioButtonGroupPlaylistAutoSavePreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    int method = (int) newValue;
                    if (mPlaylistService != null) {
                        SharedPreferencesManager.getInstance().writeInt(
                                BravePlaylistPreferences.PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE, method);
                    }
                    return true;
                });
    }
}

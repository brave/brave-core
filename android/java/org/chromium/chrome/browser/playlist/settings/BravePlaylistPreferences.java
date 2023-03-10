/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.os.Bundle;
import android.os.Handler;

import androidx.preference.Preference;

import com.brave.playlist.util.PlaylistPreferenceUtils;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistResetPreference;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistService;

public class BravePlaylistPreferences extends BravePreferenceFragment
        implements ConnectionErrorHandler, Preference.OnPreferenceChangeListener {
    private static final String PREF_PLAYLIST_PREFERENCE_SCREEN = "playlist_preference_screen";
    public static final String PREF_ENABLE_PLAYLIST = "enable_playlist";
    public static final String PREF_ADD_TO_PLAYLIST_BUTTON = "add_to_playlist_button";
    private static final String PREF_AUTO_PLAY = "auto_play";
    public static final String PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE = "auto_save_media_for_offline";
    private static final String PREF_START_PLAYBACK = "start_playback";
    private static final String PREF_RESET_PLAYLIST = "reset_playlist";

    private ChromeSwitchPreference mEnablePlaylistSwitch;
    private ChromeSwitchPreference mAddToPlaylistButtonSwitch;
    private ChromeSwitchPreference mAutoPlaySwitch;
    private Preference mAutoSaveMediaForOfflinePreference;
    private ChromeSwitchPreference mStartPlaybackSwitch;
    private BravePlaylistResetPreference mResetPlaylist;

    private PlaylistService mPlaylistService;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mEnablePlaylistSwitch = (ChromeSwitchPreference) findPreference(PREF_ENABLE_PLAYLIST);
        mEnablePlaylistSwitch.setChecked(
                SharedPreferencesManager.getInstance().readBoolean(PREF_ENABLE_PLAYLIST, true));
        mEnablePlaylistSwitch.setOnPreferenceChangeListener(this);

        mAddToPlaylistButtonSwitch =
                (ChromeSwitchPreference) findPreference(PREF_ADD_TO_PLAYLIST_BUTTON);
        mAddToPlaylistButtonSwitch.setChecked(SharedPreferencesManager.getInstance().readBoolean(
                PREF_ADD_TO_PLAYLIST_BUTTON, true));
        mAddToPlaylistButtonSwitch.setOnPreferenceChangeListener(this);

        mAutoPlaySwitch = (ChromeSwitchPreference) findPreference(PREF_AUTO_PLAY);
        mAutoPlaySwitch.setChecked(
                SharedPreferencesManager.getInstance().readBoolean(PREF_AUTO_PLAY, true));
        mAutoPlaySwitch.setOnPreferenceChangeListener(this);

        mAutoSaveMediaForOfflinePreference =
                (Preference) findPreference(PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE);
        updateAutoSaveMedia();

        mStartPlaybackSwitch = (ChromeSwitchPreference) findPreference(PREF_START_PLAYBACK);
        mStartPlaybackSwitch.setChecked(
                SharedPreferencesManager.getInstance().readBoolean(PREF_START_PLAYBACK, true));
        mStartPlaybackSwitch.setOnPreferenceChangeListener(this);

        mResetPlaylist = (BravePlaylistResetPreference) findPreference(PREF_RESET_PLAYLIST);
        mResetPlaylist.setOnPreferenceClickListener(preference -> {
            if (mPlaylistService != null) {
                mPlaylistService.resetAll();
                PlaylistPreferenceUtils.resetPlaylistPrefs(getActivity());
                BraveRelaunchUtils.askForRelaunch(getActivity());
            }
            return true;
        });

        updatePlaylistSettingsState(
                SharedPreferencesManager.getInstance().readBoolean(PREF_ENABLE_PLAYLIST, true));
    }

    @Override
    public void onDestroy() {
        if (mPlaylistService != null) {
            mPlaylistService.close();
        }
        super.onDestroy();
    }

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
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_playlist);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_playlist_preferences);
        initPlaylistService();
    }

    @Override
    public void onResume() {
        super.onResume();
        new Handler().post(() -> { updateAutoSaveMedia(); });
    }

    private void updateAutoSaveMedia() {
        if (mAutoSaveMediaForOfflinePreference != null) {
            switch (SharedPreferencesManager.getInstance().readInt(
                    PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE, 0)) {
                case 0:
                    mAutoSaveMediaForOfflinePreference.setSummary(
                            getActivity().getResources().getString(R.string.on_text));
                    break;
                case 1:
                    mAutoSaveMediaForOfflinePreference.setSummary(
                            getActivity().getResources().getString(R.string.off_text));
                    break;
                case 2:
                    mAutoSaveMediaForOfflinePreference.setSummary(
                            getActivity().getResources().getString(R.string.wifi_only_text));
                    break;
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_ENABLE_PLAYLIST.equals(key)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    PREF_ENABLE_PLAYLIST, (boolean) newValue);
            updatePlaylistSettingsState((boolean) newValue);
        } else if (PREF_ADD_TO_PLAYLIST_BUTTON.equals(key)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    PREF_ADD_TO_PLAYLIST_BUTTON, (boolean) newValue);
        } else if (PREF_AUTO_PLAY.equals(key)) {
            SharedPreferencesManager.getInstance().writeBoolean(PREF_AUTO_PLAY, (boolean) newValue);
        } else if (PREF_START_PLAYBACK.equals(key)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    PREF_START_PLAYBACK, (boolean) newValue);
        }

        return true;
    }

    private void updatePlaylistSettingsState(boolean isPlaylistEnabled) {
        if (isPlaylistEnabled) {
            if (mAddToPlaylistButtonSwitch != null) {
                getPreferenceScreen().addPreference(mAddToPlaylistButtonSwitch);
            }
            if (mAutoPlaySwitch != null) {
                getPreferenceScreen().addPreference(mAutoPlaySwitch);
            }
            if (mAutoSaveMediaForOfflinePreference != null) {
                getPreferenceScreen().addPreference(mAutoSaveMediaForOfflinePreference);
            }
            if (mStartPlaybackSwitch != null) {
                getPreferenceScreen().addPreference(mStartPlaybackSwitch);
            }
            if (mResetPlaylist != null) {
                getPreferenceScreen().addPreference(mResetPlaylist);
            }
        } else {
            if (mAddToPlaylistButtonSwitch != null) {
                getPreferenceScreen().removePreference(mAddToPlaylistButtonSwitch);
            }
            if (mAutoPlaySwitch != null) {
                getPreferenceScreen().removePreference(mAutoPlaySwitch);
            }
            if (mAutoSaveMediaForOfflinePreference != null) {
                getPreferenceScreen().removePreference(mAutoSaveMediaForOfflinePreference);
            }
            if (mStartPlaybackSwitch != null) {
                getPreferenceScreen().removePreference(mStartPlaybackSwitch);
            }
            if (mResetPlaylist != null) {
                getPreferenceScreen().removePreference(mResetPlaylist);
            }
        }
    }
}

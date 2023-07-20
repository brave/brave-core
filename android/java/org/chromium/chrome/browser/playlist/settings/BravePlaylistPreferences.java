/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.os.Bundle;
import android.os.Handler;

import androidx.preference.Preference;

import com.brave.playlist.util.PlaylistPreferenceUtils;

import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
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
    public static final String PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE = "auto_save_media_for_offline";
    private static final String PREF_REMEMBER_FILE_PLAYBACK_POSITION =
            "remember_file_playback_position";
    private static final String PREF_REMEMBER_LIST_PLAYBACK_POSITION =
            "remember_list_playback_position";
    private static final String PREF_CONTINUOUS_LISTENING = "continuous_listening";
    private static final String PREF_RESET_PLAYLIST = "reset_playlist";

    private ChromeSwitchPreference mEnablePlaylistSwitch;
    private ChromeSwitchPreference mAddToPlaylistButtonSwitch;
    private Preference mAutoSaveMediaForOfflinePreference;
    private ChromeSwitchPreference mRememberFilePlaybackPositionSwitch;
    private ChromeSwitchPreference mRememberListPlaybackPositionSwitch;
    private ChromeSwitchPreference mContinuousListeningSwitch;
    private BravePlaylistResetPreference mResetPlaylist;

    private PlaylistService mPlaylistService;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mEnablePlaylistSwitch = (ChromeSwitchPreference) findPreference(PREF_ENABLE_PLAYLIST);
        mEnablePlaylistSwitch.setOnPreferenceChangeListener(this);

        mAddToPlaylistButtonSwitch =
                (ChromeSwitchPreference) findPreference(PREF_ADD_TO_PLAYLIST_BUTTON);
        mAddToPlaylistButtonSwitch.setOnPreferenceChangeListener(this);

        mAutoSaveMediaForOfflinePreference =
                (Preference) findPreference(PREF_AUTO_SAVE_MEDIA_FOR_OFFLINE);
        updateAutoSaveMedia();

        mRememberFilePlaybackPositionSwitch =
                (ChromeSwitchPreference) findPreference(PREF_REMEMBER_FILE_PLAYBACK_POSITION);
        mRememberListPlaybackPositionSwitch =
                (ChromeSwitchPreference) findPreference(PREF_REMEMBER_LIST_PLAYBACK_POSITION);
        mContinuousListeningSwitch =
                (ChromeSwitchPreference) findPreference(PREF_CONTINUOUS_LISTENING);

        mResetPlaylist = (BravePlaylistResetPreference) findPreference(PREF_RESET_PLAYLIST);
        mResetPlaylist.setOnPreferenceClickListener(preference -> {
            if (mPlaylistService != null) {
                PostTask.postTask(TaskTraits.USER_VISIBLE_MAY_BLOCK, () -> {
                    mPlaylistService.resetAll();
                    PlaylistPreferenceUtils.resetPlaylistPrefs(getActivity());
                    getActivity().runOnUiThread(
                            (Runnable) () -> BraveRelaunchUtils.askForRelaunch(getActivity()));
                });
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
        if (mAutoSaveMediaForOfflinePreference == null) {
            return;
        }
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

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_ENABLE_PLAYLIST.equals(key)) {
            updatePlaylistSettingsState((boolean) newValue);
        }

        return true;
    }

    private void updatePlaylistSettingsState(boolean isPlaylistEnabled) {
        if (isPlaylistEnabled) {
            if (mAddToPlaylistButtonSwitch != null) {
                getPreferenceScreen().addPreference(mAddToPlaylistButtonSwitch);
            }
            if (mAutoSaveMediaForOfflinePreference != null) {
                getPreferenceScreen().addPreference(mAutoSaveMediaForOfflinePreference);
            }
            if (mRememberFilePlaybackPositionSwitch != null) {
                getPreferenceScreen().addPreference(mRememberFilePlaybackPositionSwitch);
            }
            if (mRememberListPlaybackPositionSwitch != null) {
                getPreferenceScreen().addPreference(mRememberListPlaybackPositionSwitch);
            }
            if (mContinuousListeningSwitch != null) {
                getPreferenceScreen().addPreference(mContinuousListeningSwitch);
            }
            if (mResetPlaylist != null) {
                getPreferenceScreen().addPreference(mResetPlaylist);
            }
        } else {
            if (mAddToPlaylistButtonSwitch != null) {
                getPreferenceScreen().removePreference(mAddToPlaylistButtonSwitch);
            }
            if (mAutoSaveMediaForOfflinePreference != null) {
                getPreferenceScreen().removePreference(mAutoSaveMediaForOfflinePreference);
            }
            if (mRememberFilePlaybackPositionSwitch != null) {
                getPreferenceScreen().removePreference(mRememberFilePlaybackPositionSwitch);
            }
            if (mRememberListPlaybackPositionSwitch != null) {
                getPreferenceScreen().removePreference(mRememberListPlaybackPositionSwitch);
            }
            if (mContinuousListeningSwitch != null) {
                getPreferenceScreen().removePreference(mContinuousListeningSwitch);
            }
            if (mResetPlaylist != null) {
                getPreferenceScreen().removePreference(mResetPlaylist);
            }
        }
    }
}

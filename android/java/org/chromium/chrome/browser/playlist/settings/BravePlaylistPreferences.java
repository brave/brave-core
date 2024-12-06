/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.local_database.PlaylistRepository;
import org.chromium.chrome.browser.playlist.util.PlaylistPreferenceUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistService;

public class BravePlaylistPreferences extends ChromeBaseSettingsFragment
        implements ConnectionErrorHandler, Preference.OnPreferenceChangeListener {
    private ChromeSwitchPreference mEnablePlaylistSwitch;
    private ChromeSwitchPreference mAddToPlaylistButtonSwitch;
    private ChromeSwitchPreference mRememberFilePlaybackPositionSwitch;
    private ChromeSwitchPreference mRememberListPlaybackPositionSwitch;
    private ChromeSwitchPreference mContinuousListeningSwitch;
    private BravePlaylistResetPreference mResetPlaylist;

    private PlaylistService mPlaylistService;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mEnablePlaylistSwitch =
                (ChromeSwitchPreference) findPreference(BravePreferenceKeys.PREF_ENABLE_PLAYLIST);
        mEnablePlaylistSwitch.setOnPreferenceChangeListener(this);

        mAddToPlaylistButtonSwitch =
                (ChromeSwitchPreference)
                        findPreference(BravePreferenceKeys.PREF_ADD_TO_PLAYLIST_BUTTON);
        mAddToPlaylistButtonSwitch.setOnPreferenceChangeListener(this);

        mRememberFilePlaybackPositionSwitch =
                (ChromeSwitchPreference)
                        findPreference(BravePreferenceKeys.PREF_REMEMBER_FILE_PLAYBACK_POSITION);
        mRememberListPlaybackPositionSwitch =
                (ChromeSwitchPreference)
                        findPreference(BravePreferenceKeys.PREF_REMEMBER_LIST_PLAYBACK_POSITION);
        mContinuousListeningSwitch =
                (ChromeSwitchPreference)
                        findPreference(BravePreferenceKeys.PREF_CONTINUOUS_LISTENING);

        mResetPlaylist =
                (BravePlaylistResetPreference)
                        findPreference(BravePreferenceKeys.PREF_RESET_PLAYLIST);
        mResetPlaylist.setOnPreferenceClickListener(
                preference -> {
                    if (mPlaylistService != null) {
                        PostTask.postTask(
                                TaskTraits.USER_VISIBLE_MAY_BLOCK,
                                () -> {
                                    mPlaylistService.clearAllQueries();
                                    mPlaylistService.resetAll();
                                    PlaylistRepository playlistRepository =
                                            new PlaylistRepository(getActivity());
                                    playlistRepository.deleteAllLastPlayedPosition();
                                    playlistRepository.deleteAllHlsContentQueueModel();
                                    PlaylistPreferenceUtils.resetPlaylistPrefs(getActivity());
                                    getActivity()
                                            .runOnUiThread(
                                                    (Runnable)
                                                            () ->
                                                                    BraveRelaunchUtils
                                                                            .askForRelaunch(
                                                                                    getActivity()));
                                });
                    }
                    return true;
                });

        updatePlaylistSettingsState(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true));
    }

    @Override
    public void onDestroy() {
        if (mPlaylistService != null) {
            mPlaylistService.close();
            mPlaylistService = null;
        }
        super.onDestroy();
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mPlaylistService != null) {
            mPlaylistService.close();
            mPlaylistService = null;
        }
        mPlaylistService = null;
        initPlaylistService();
    }

    private void initPlaylistService() {
        if (mPlaylistService != null) {
            return;
        }

        mPlaylistService =
                PlaylistServiceFactoryAndroid.getInstance().getPlaylistService(getProfile(), this);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mPageTitle.set(getString(R.string.brave_playlist));
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_playlist_preferences);
        initPlaylistService();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (BravePreferenceKeys.PREF_ENABLE_PLAYLIST.equals(key)) {
            updatePlaylistSettingsState((boolean) newValue);
        }

        return true;
    }

    private void updatePlaylistSettingsState(boolean isPlaylistEnabled) {
        if (isPlaylistEnabled) {
            if (mAddToPlaylistButtonSwitch != null) {
                getPreferenceScreen().addPreference(mAddToPlaylistButtonSwitch);
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

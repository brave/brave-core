/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistResetPreference;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BravePlaylistPreferences
        extends BravePreferenceFragment implements Preference.OnPreferenceChangeListener {
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
        mAutoSaveMediaForOfflinePreference.setSummary("On");

        mStartPlaybackSwitch = (ChromeSwitchPreference) findPreference(PREF_START_PLAYBACK);
        mStartPlaybackSwitch.setChecked(
                SharedPreferencesManager.getInstance().readBoolean(PREF_START_PLAYBACK, true));
        mStartPlaybackSwitch.setOnPreferenceChangeListener(this);

        mResetPlaylist = (BravePlaylistResetPreference) findPreference(PREF_RESET_PLAYLIST);
        mResetPlaylist.setOnPreferenceChangeListener(this);

        updatePlaylistSettingsState(
                SharedPreferencesManager.getInstance().readBoolean(PREF_ENABLE_PLAYLIST, true));
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_playlist);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_playlist_preferences);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_ENABLE_PLAYLIST.equals(key)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    PREF_ENABLE_PLAYLIST, (boolean) newValue);
            updatePlaylistSettingsState((boolean) newValue);
            BraveRelaunchUtils.askForRelaunch(getActivity());
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

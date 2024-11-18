/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.util

import android.content.Context
import android.content.SharedPreferences
import androidx.preference.PreferenceManager

object PlaylistPreferenceUtils {
    private const val RECENTLY_PLAYED_PLAYLIST = "recently_played_playlist"
    const val SHOULD_SHOW_PLAYLIST_ONBOARDING = "should_show_playlist_onboarding"
    const val ADD_MEDIA_COUNT = "add_media_count"
    private const val REMEMBER_FILE_PLAYBACK_POSITION = "remember_file_playback_position"
    private const val REMEMBER_LIST_PLAYBACK_POSITION = "remember_list_playback_position"
    private const val CONTINUOUS_LISTENING = "continuous_listening"

    fun defaultPrefs(context: Context): SharedPreferences =
        PreferenceManager.getDefaultSharedPreferences(context)

    private inline fun SharedPreferences.edit(operation: (SharedPreferences.Editor) -> Unit) {
        val editor = this.edit()
        operation(editor)
        editor.apply()
    }

    var SharedPreferences.recentlyPlayedPlaylist
        get() = getString(RECENTLY_PLAYED_PLAYLIST, "")
        set(value) {
            edit {
                it.putString(RECENTLY_PLAYED_PLAYLIST, value)
            }
        }

    private var SharedPreferences.addMediaCount
        get() = getInt(ADD_MEDIA_COUNT, -1)
        set(value) {
            edit {
                it.putInt(ADD_MEDIA_COUNT, value)
            }
        }

    var SharedPreferences.shouldShowOnboarding
        get() = getBoolean(SHOULD_SHOW_PLAYLIST_ONBOARDING, true)
        set(value) {
            edit {
                it.putBoolean(SHOULD_SHOW_PLAYLIST_ONBOARDING, value)
            }
        }

    fun SharedPreferences.getLatestPlaylistItem(key: String): String? = getString(key, "")

    fun SharedPreferences.setLatestPlaylistItem(key: String, value: String) {
        edit {
            it.putString(key, value)
        }
    }

    var SharedPreferences.rememberFilePlaybackPosition
        get() = getBoolean(REMEMBER_FILE_PLAYBACK_POSITION, true)
        set(value) {
            edit {
                it.putBoolean(REMEMBER_FILE_PLAYBACK_POSITION, value)
            }
        }

    var SharedPreferences.rememberListPlaybackPosition
        get() = getBoolean(REMEMBER_LIST_PLAYBACK_POSITION, false)
        set(value) {
            edit {
                it.putBoolean(REMEMBER_LIST_PLAYBACK_POSITION, value)
            }
        }

    var SharedPreferences.continuousListening
        get() = getBoolean(CONTINUOUS_LISTENING, true)
        set(value) {
            edit {
                it.putBoolean(CONTINUOUS_LISTENING, value)
            }
        }

    @JvmStatic
    @Suppress("unused")
    fun resetPlaylistPrefs(context: Context) {
        defaultPrefs(context).apply {
            recentlyPlayedPlaylist = ""
            shouldShowOnboarding = true
            addMediaCount = -1
            rememberFilePlaybackPosition = true
            rememberListPlaybackPosition = false
            continuousListening = true
        }
    }
}

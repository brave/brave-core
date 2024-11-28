/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.util

import android.content.Context
import android.view.View
import androidx.fragment.app.FragmentManager

import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistBaseActivity
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemOptionModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.PlaylistItemOptionsBottomSheet
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.PlaylistOptionsBottomSheet
import org.chromium.playlist.mojom.Playlist
import org.chromium.playlist.mojom.PlaylistItem

object MenuUtils {
    @JvmStatic
    fun showPlaylistItemMenu(
        context: Context,
        fragmentManager: FragmentManager,
        playlistItem: PlaylistItem,
        playlistId: String?,
        playlistItemOptionsListener: PlaylistItemOptionsListener,
        shouldShowMove: Boolean = true,
    ) {
        val optionsList: MutableList<PlaylistItemOptionModel> = mutableListOf()

        if (shouldShowMove) {
            optionsList.add(
                PlaylistItemOptionModel(
                    context.resources.getString(R.string.playlist_move_item),
                    R.drawable.ic_move_media,
                    PlaylistBaseActivity.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM,
                    playlistItem = playlistItem,
                    playlistId = playlistId
                )
            )
        }
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_copy_item),
                R.drawable.ic_copy_media,
                PlaylistBaseActivity.PlaylistOptionsEnum.COPY_PLAYLIST_ITEM,
                playlistItem = playlistItem,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_share_item),
                R.drawable.ic_share,
                PlaylistBaseActivity.PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM,
                playlistItem = playlistItem,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_open_in_new_tab),
                R.drawable.ic_new_tab,
                PlaylistBaseActivity.PlaylistOptionsEnum.OPEN_IN_NEW_TAB,
                playlistItem = playlistItem,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_open_in_private_tab),
                R.drawable.ic_private_tab,
                PlaylistBaseActivity.PlaylistOptionsEnum.OPEN_IN_PRIVATE_TAB,
                playlistItem = playlistItem,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_delete_item),
                R.drawable.ic_playlist_delete,
                PlaylistBaseActivity.PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM,
                playlistItem = playlistItem,
                playlistId = playlistId
            )
        )
        PlaylistItemOptionsBottomSheet(optionsList, playlistItemOptionsListener)
            .show(fragmentManager, null)
    }

    @JvmStatic
    fun showMoveOrCopyMenu(
        view: View,
        fragmentManager: FragmentManager,
        selectedPlaylistItems: ArrayList<PlaylistItem>,
        playlistOptionsListener: PlaylistOptionsListener
    ) {
        PlaylistOptionsBottomSheet(
                mutableListOf(
                    PlaylistOptionsModel(
                        view.resources.getString(R.string.playlist_move_item),
                        R.drawable.ic_move_media,
                        PlaylistBaseActivity.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS,
                        playlistItems = selectedPlaylistItems
                    ),
                    PlaylistOptionsModel(
                        view.resources.getString(R.string.playlist_copy_item),
                        R.drawable.ic_copy_media,
                        PlaylistBaseActivity.PlaylistOptionsEnum.COPY_PLAYLIST_ITEMS,
                        playlistItems = selectedPlaylistItems
                    )
                ),
                playlistOptionsListener
            )
            .show(fragmentManager, null)
    }

    @JvmStatic
    fun showPlaylistMenu(
        context: Context,
        fragmentManager: FragmentManager,
        playlist: Playlist,
        playlistOptionsListener: PlaylistOptionsListener,
        isDefaultPlaylist: Boolean = false
    ) {

        val optionsList: MutableList<PlaylistOptionsModel> = mutableListOf()
        optionsList.add(
            PlaylistOptionsModel(
                context.resources.getString(R.string.playlist_edit_text),
                R.drawable.ic_edit_playlist,
                PlaylistBaseActivity.PlaylistOptionsEnum.EDIT_PLAYLIST,
                playlist = playlist
            )
        )
        if (!isDefaultPlaylist) {
            optionsList.add(
                PlaylistOptionsModel(
                    context.resources.getString(R.string.playlist_rename_text),
                    R.drawable.ic_rename_playlist,
                    PlaylistBaseActivity.PlaylistOptionsEnum.RENAME_PLAYLIST,
                    playlist = playlist
                )
            )
        }
        if (!isDefaultPlaylist) {
            optionsList.add(
                PlaylistOptionsModel(
                    context.resources.getString(R.string.playlist_delete_playlist),
                    R.drawable.ic_playlist_delete,
                    PlaylistBaseActivity.PlaylistOptionsEnum.DELETE_PLAYLIST,
                    playlist = playlist
                )
            )
        }
        PlaylistOptionsBottomSheet(optionsList, playlistOptionsListener).show(fragmentManager, null)
    }
}

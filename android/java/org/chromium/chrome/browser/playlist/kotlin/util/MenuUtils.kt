/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.util

import android.content.Context
import android.view.View
import androidx.fragment.app.FragmentManager
import org.chromium.chrome.browser.playlist.R
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemOptionModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.PlaylistItemOptionsBottomSheet
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.PlaylistOptionsBottomSheet

object MenuUtils {
    @JvmStatic
    fun showPlaylistItemMenu(
        context: Context,
        fragmentManager: FragmentManager,
        playlistItemModel: PlaylistItemModel,
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
                    PlaylistModel.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM,
                    playlistItemModel = playlistItemModel,
                    playlistId = playlistId
                )
            )
        }
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_copy_item),
                R.drawable.ic_copy_media,
                PlaylistModel.PlaylistOptionsEnum.COPY_PLAYLIST_ITEM,
                playlistItemModel = playlistItemModel,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_share_item),
                R.drawable.ic_share,
                PlaylistModel.PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM,
                playlistItemModel = playlistItemModel,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_open_in_new_tab),
                R.drawable.ic_new_tab,
                PlaylistModel.PlaylistOptionsEnum.OPEN_IN_NEW_TAB,
                playlistItemModel = playlistItemModel,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_open_in_private_tab),
                R.drawable.ic_private_tab,
                PlaylistModel.PlaylistOptionsEnum.OPEN_IN_PRIVATE_TAB,
                playlistItemModel = playlistItemModel,
                playlistId = playlistId
            )
        )
        optionsList.add(
            PlaylistItemOptionModel(
                context.resources.getString(R.string.playlist_delete_item),
                R.drawable.ic_playlist_delete,
                PlaylistModel.PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM,
                playlistItemModel = playlistItemModel,
                playlistId = playlistId
            )
        )
        PlaylistItemOptionsBottomSheet(
            optionsList, playlistItemOptionsListener
        ).show(fragmentManager, null)
    }

    @JvmStatic
    fun showMoveOrCopyMenu(
        view: View,
        fragmentManager: FragmentManager,
        selectedItems: ArrayList<PlaylistItemModel>,
        playlistOptionsListener: PlaylistOptionsListener
    ) {
        PlaylistOptionsBottomSheet(
            mutableListOf(
                PlaylistOptionsModel(
                    view.resources.getString(R.string.playlist_move_item),
                    R.drawable.ic_move_media,
                    PlaylistModel.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS,
                    playlistItemModels = selectedItems
                ), PlaylistOptionsModel(
                    view.resources.getString(R.string.playlist_copy_item),
                    R.drawable.ic_copy_media,
                    PlaylistModel.PlaylistOptionsEnum.COPY_PLAYLIST_ITEMS,
                    playlistItemModels = selectedItems
                )
            ), playlistOptionsListener
        ).show(fragmentManager, null)
    }

    @JvmStatic
    fun showPlaylistMenu(
        context: Context,
        fragmentManager: FragmentManager,
        playlistModel: PlaylistModel,
        playlistOptionsListener: PlaylistOptionsListener,
        isDefaultPlaylist: Boolean = false
    ) {

        val optionsList: MutableList<PlaylistOptionsModel> = mutableListOf()
        optionsList.add(
            PlaylistOptionsModel(
                context.resources.getString(R.string.playlist_edit_text),
                R.drawable.ic_edit_playlist,
                PlaylistModel.PlaylistOptionsEnum.EDIT_PLAYLIST,
                playlistModel = playlistModel
            )
        )
        if (!isDefaultPlaylist) {
            optionsList.add(
                PlaylistOptionsModel(
                    context.resources.getString(R.string.playlist_rename_text),
                    R.drawable.ic_rename_playlist,
                    PlaylistModel.PlaylistOptionsEnum.RENAME_PLAYLIST,
                    playlistModel = playlistModel
                )
            )
        }
        if (!isDefaultPlaylist) {
            optionsList.add(
                PlaylistOptionsModel(
                    context.resources.getString(R.string.playlist_delete_playlist),
                    R.drawable.ic_playlist_delete,
                    PlaylistModel.PlaylistOptionsEnum.DELETE_PLAYLIST,
                    playlistModel = playlistModel
                )
            )
        }
        PlaylistOptionsBottomSheet(
            optionsList, playlistOptionsListener
        ).show(fragmentManager, null)
    }
}

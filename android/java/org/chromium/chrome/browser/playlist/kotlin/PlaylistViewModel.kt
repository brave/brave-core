/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.brave.playlist.model.CreatePlaylistModel
import com.brave.playlist.model.HlsContentProgressModel
import com.brave.playlist.model.MoveOrCopyModel
import com.brave.playlist.model.PlaylistItemModel
import com.brave.playlist.model.PlaylistItemOptionModel
import com.brave.playlist.model.PlaylistModel
import com.brave.playlist.model.PlaylistOptionsModel
import com.brave.playlist.model.RenamePlaylistModel

class PlaylistViewModel : ViewModel() {
    // Using Livedata for Playlist Data
    private val mutablePlaylistData = MutableLiveData<PlaylistModel>()
    val playlistData: LiveData<PlaylistModel> get() = mutablePlaylistData
    fun setPlaylistData(playlistModel: PlaylistModel) {
        mutablePlaylistData.value = playlistModel
    }

    // Using Livedata for all Playlist Data
    private val mutableAllPlaylistData = MutableLiveData<List<PlaylistModel>>()
    val allPlaylistData: LiveData<List<PlaylistModel>> get() = mutableAllPlaylistData
    fun setAllPlaylistData(allPlaylistData: List<PlaylistModel>) {
        mutableAllPlaylistData.value = allPlaylistData
    }

    // Using after creating new playlist
    private val mutableCreatePlaylistOption = MutableLiveData<CreatePlaylistModel>()
    val createPlaylistOption: LiveData<CreatePlaylistModel> get() = mutableCreatePlaylistOption
    fun setCreatePlaylistOption(createPlaylistModel: CreatePlaylistModel) {
        mutableCreatePlaylistOption.value = createPlaylistModel
    }

    // Using Livedata to open specific Playlist
    private val mutablePlaylistToOpen = MutableLiveData<String>()
    val playlistToOpen: LiveData<String> get() = mutablePlaylistToOpen
    fun setPlaylistToOpen(playlistId: String) {
        mutablePlaylistToOpen.value = playlistId
    }

    // Using Livedata for playlist menu option
    private val mutablePlaylistOption = MutableLiveData<PlaylistOptionsModel>()
    val playlistOption: LiveData<PlaylistOptionsModel> get() = mutablePlaylistOption
    fun setPlaylistOption(playlistOptionsModel: PlaylistOptionsModel) {
        mutablePlaylistOption.value = playlistOptionsModel
    }

    // Using Livedata for all playlist menu option
    private val mutableAllPlaylistOption = MutableLiveData<PlaylistOptionsModel>()
    val allPlaylistOption: LiveData<PlaylistOptionsModel> get() = mutableAllPlaylistOption
    fun setAllPlaylistOption(playlistOptionsModel: PlaylistOptionsModel) {
        mutableAllPlaylistOption.value = playlistOptionsModel
    }

    // Using Livedata for playlist item menu option
    private val mutablePlaylistItemOption = MutableLiveData<PlaylistItemOptionModel>()
    val playlistItemOption: LiveData<PlaylistItemOptionModel> get() = mutablePlaylistItemOption
    fun setPlaylistItemOption(playlistItemOptionModel: PlaylistItemOptionModel) {
        mutablePlaylistItemOption.value = playlistItemOptionModel
    }

    // Using Livedata for playlist multiple items menu option
    private val mutablePlaylistMultipleItemOption = MutableLiveData<PlaylistOptionsModel>()
    val playlistMultipleItemOption: LiveData<PlaylistOptionsModel> get() = mutablePlaylistMultipleItemOption
    fun setPlaylistMultipleItemOption(playlistMultipleItemOptionModel: PlaylistOptionsModel) {
        mutablePlaylistMultipleItemOption.value = playlistMultipleItemOptionModel
    }

    // Using Livedata to delete multiple items from a playlist
    private val mutableDeletePlaylistItems = MutableLiveData<PlaylistModel>()
    val deletePlaylistItems: LiveData<PlaylistModel> get() = mutableDeletePlaylistItems
    fun setDeletePlaylistItems(playlistItems: PlaylistModel) {
        mutableDeletePlaylistItems.value = playlistItems
    }

    private val mutableRenamePlaylistOption = MutableLiveData<RenamePlaylistModel>()
    val renamePlaylistOption: LiveData<RenamePlaylistModel> get() = mutableRenamePlaylistOption
    fun setRenamePlaylistOption(renamePlaylistModel: RenamePlaylistModel) {
        mutableRenamePlaylistOption.value = renamePlaylistModel
    }

    private val mutableDownloadProgress = MutableLiveData<HlsContentProgressModel>()
    val downloadProgress: LiveData<HlsContentProgressModel> get() = mutableDownloadProgress
    fun updateDownloadProgress(hlsContentProgressModel: HlsContentProgressModel) {
        mutableDownloadProgress.value = hlsContentProgressModel
    }

    private val mutablePlaylistItemUpdate = MutableLiveData<PlaylistItemModel>()
    val playlistItemUpdate: LiveData<PlaylistItemModel> get() = mutablePlaylistItemUpdate
    fun updatePlaylistItem(playlistItemModel: PlaylistItemModel) {
        mutablePlaylistItemUpdate.value = playlistItemModel
    }

    private val mutableFetchPlaylistData = MutableLiveData<String>()
    val fetchPlaylistData: LiveData<String> get() = mutableFetchPlaylistData
    fun fetchPlaylistData(playlistId: String) {
        mutableFetchPlaylistData.value = playlistId
    }

    private val mutableReorderPlaylistItems = MutableLiveData<List<PlaylistItemModel>>()
    val reorderPlaylistItems: LiveData<List<PlaylistItemModel>> get() = mutableReorderPlaylistItems
    fun reorderPlaylistItems(playlistItems: MutableList<PlaylistItemModel>) {
        mutableReorderPlaylistItems.value = playlistItems
    }

    private val mutableMoveOrCopyItems = MutableLiveData<MoveOrCopyModel>()
    val moveOrCopyItems: LiveData<MoveOrCopyModel> get() = mutableMoveOrCopyItems
    fun performMoveOrCopy(moveOrCopyModel: MoveOrCopyModel) {
        mutableMoveOrCopyItems.value = moveOrCopyModel
    }

    private val mutableDefaultPlaylist = MutableLiveData<String>()
    val defaultPlaylist: LiveData<String> get() = mutableDefaultPlaylist
    fun setDefaultPlaylist(playlistId: String) {
        mutableDefaultPlaylist.value = playlistId
    }

    private val mutableStartDownloadingFromQueue = MutableLiveData(false)
    val startDownloadingFromQueue: LiveData<Boolean> get() = mutableStartDownloadingFromQueue
    fun startDownloadingFromQueue(shouldStartDownload: Boolean) {
        mutableStartDownloadingFromQueue.postValue(shouldStartDownload)
    }

    private val mutablePlaylistItemRemove = MutableLiveData<String>()
    val playlistItemRemove: LiveData<String> get() = mutablePlaylistItemRemove
    fun removePlaylistItem(playlistItemId: String) {
        mutablePlaylistItemRemove.value = playlistItemId
    }
}

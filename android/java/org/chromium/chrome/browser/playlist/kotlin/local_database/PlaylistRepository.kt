/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.local_database

import android.content.Context
import org.chromium.chrome.browser.playlist.kotlin.model.HlsContentQueueModel
import org.chromium.chrome.browser.playlist.kotlin.model.LastPlayedPositionModel

class PlaylistRepository(context: Context) {
    companion object {
        val TAG: String = "Playlist/" + this::class.java.simpleName
    }

    private var mPlaylistItemModelDao: PlaylistItemModelDao? =
        PlaylistDatabase.getInstance(context)?.playlistItemModelDao()

    fun getLastPlayedPositionByPlaylistItemId(playlistItemId: String): LastPlayedPositionModel? {
        return mPlaylistItemModelDao?.getLastPlayedPositionByPlaylistItemId(playlistItemId)
    }

    fun insertLastPlayedPosition(lastPlayedPositionModel: LastPlayedPositionModel) {
        mPlaylistItemModelDao?.insertLastPlayedPosition(lastPlayedPositionModel)
    }

    fun deleteAllLastPlayedPosition() {
        mPlaylistItemModelDao?.deleteAllLastPlayedPosition()
    }

    fun isHlsContentQueueModelExists(playlistItemId: String): Boolean? {
        return mPlaylistItemModelDao?.isHlsContentQueueModelExists(playlistItemId)
    }

    @Suppress("unused")
    fun updateHlsContentQueueModel(hlsContentQueueModel: HlsContentQueueModel) {
        mPlaylistItemModelDao?.updateHlsContentQueueModel(hlsContentQueueModel)
    }

    @Suppress("unused")
    fun getFirstHlsContentQueueModel(): HlsContentQueueModel? {
        return mPlaylistItemModelDao?.getFirstHlsContentQueueModel()
    }

    fun getAllHlsContentQueueModel(): List<HlsContentQueueModel>? {
        return mPlaylistItemModelDao?.getAllHlsContentQueueModel()
    }

    fun insertHlsContentQueueModel(hlsContentQueueModel: HlsContentQueueModel) {
        mPlaylistItemModelDao?.insertHlsContentQueueModel(hlsContentQueueModel)
    }

    fun deleteHlsContentQueueModel(playlistItemId: String) {
        mPlaylistItemModelDao?.deleteHlsContentQueueModel(playlistItemId)
    }

    fun deleteAllHlsContentQueueModel() {
        mPlaylistItemModelDao?.deleteAllHlsContentQueueModel()
    }
}

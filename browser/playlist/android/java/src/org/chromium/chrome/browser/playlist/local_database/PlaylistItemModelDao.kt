/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.local_database

import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import org.chromium.chrome.browser.playlist.model.HlsContentQueueModel
import org.chromium.chrome.browser.playlist.model.LastPlayedPositionModel

@Dao
interface PlaylistItemModelDao {
    @Query("SELECT * FROM LastPlayedPositionModel WHERE playlist_item_id = :playlistItemId LIMIT 1")
    fun getLastPlayedPositionByPlaylistItemId(playlistItemId: String): LastPlayedPositionModel

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insertLastPlayedPosition(vararg lastPlayedPositionModel: LastPlayedPositionModel)

    @Query("DELETE FROM LastPlayedPositionModel")
    fun deleteAllLastPlayedPosition()

    // HlsContent queue models
    @Query("SELECT * FROM HlsContentQueueModel")
    fun getAllHlsContentQueueModel(): List<HlsContentQueueModel>

    @Query("SELECT * FROM HlsContentQueueModel WHERE hls_content_status = 'NOT_READY' LIMIT 1")
    fun getFirstHlsContentQueueModel(): HlsContentQueueModel

    @Query("SELECT * FROM HlsContentQueueModel WHERE playlist_item_id = :playlistItemId LIMIT 1")
    fun getHlsContentQueueModelById(playlistItemId: String): HlsContentQueueModel

    @Query("SELECT EXISTS (SELECT 1 FROM HlsContentQueueModel WHERE playlist_item_id = :playlistItemId)")
    fun isHlsContentQueueModelExists(playlistItemId: String): Boolean

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insertHlsContentQueueModel(vararg hlsContentQueueModel: HlsContentQueueModel)

    @Update
    fun updateHlsContentQueueModel(vararg hlsContentQueueModel: HlsContentQueueModel)

    @Delete
    fun deleteHlsContentQueueModel(vararg hlsContentQueueModel: HlsContentQueueModel)

    @Query("DELETE FROM HlsContentQueueModel WHERE playlist_item_id = :playlistItemId")
    fun deleteHlsContentQueueModel(playlistItemId: String)

    @Query("DELETE FROM HlsContentQueueModel")
    fun deleteAllHlsContentQueueModel()

}

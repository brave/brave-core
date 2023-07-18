/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.local_database

import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel

@Dao
interface PlaylistItemModelDao {
    @Query("SELECT * FROM PlaylistItemModel")
    fun getAll(): List<PlaylistItemModel>

    @Query("SELECT * FROM PlaylistItemModel WHERE id = :playlistItemId LIMIT 1")
    fun getPlaylistItemById(playlistItemId: String): PlaylistItemModel

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insertPlaylistItemModel(vararg playlistItemModel: PlaylistItemModel)

    @Update
    fun updatePlaylistItemModel(vararg playlistItemModel: PlaylistItemModel)

    @Delete
    fun deletePlaylistItemModel(vararg playlistItemModel: PlaylistItemModel)

    @Query("DELETE FROM PlaylistItemModel")
    fun deleteAllPlaylistItemModel()
}
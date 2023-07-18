/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.model

import android.os.Parcel
import android.os.Parcelable
import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey
import com.google.gson.annotations.SerializedName

@Entity
data class PlaylistItemModel(
    @PrimaryKey val id: String,
    @SerializedName("playlist_id") @ColumnInfo(name = "playlist_id") val playlistId: String,
    @SerializedName("name") val name: String,
    @SerializedName("page_source") @ColumnInfo(name = "page_source") val pageSource: String,
    @SerializedName("media_path") @ColumnInfo(name = "media_path") val mediaPath: String,
    @SerializedName("media_src") @ColumnInfo(name = "media_src") val mediaSrc: String,
    @SerializedName("thumbnail_path") @ColumnInfo(name = "thumbnail_path") val thumbnailPath: String,
    @SerializedName("author") val author: String,
    @SerializedName("duration") val duration: String,
    @SerializedName("last_played_position") @ColumnInfo(name = "last_played_position") var lastPlayedPosition: Long,
    @SerializedName("is_cached") @ColumnInfo(name = "is_cached") val isCached: Boolean = false,
    @SerializedName("is_selected") @ColumnInfo(name = "is_selected") var isSelected: Boolean = false,
    @SerializedName("file_size") @ColumnInfo(name = "file_size") var fileSize: Long = 0,
) : Parcelable {
    companion object {
        @JvmField
        val CREATOR = object : Parcelable.Creator<PlaylistItemModel> {
            override fun createFromParcel(parcel: Parcel) = PlaylistItemModel(parcel)
            override fun newArray(size: Int) = arrayOfNulls<PlaylistItemModel>(size)
        }
    }

    private constructor(parcel: Parcel) : this(
        id = parcel.readString().toString(),
        playlistId = parcel.readString().toString(),
        name = parcel.readString().toString(),
        pageSource = parcel.readString().toString(),
        mediaPath = parcel.readString().toString(),
        mediaSrc = parcel.readString().toString(),
        thumbnailPath = parcel.readString().toString(),
        author = parcel.readString().toString(),
        duration = parcel.readString().toString(),
        lastPlayedPosition = parcel.readLong(),
        isCached = parcel.readInt() == 1,
        isSelected = parcel.readInt() == 1,
        fileSize = parcel.readLong(),
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeString(id)
        parcel.writeString(playlistId)
        parcel.writeString(name)
        parcel.writeString(pageSource)
        parcel.writeString(mediaPath)
        parcel.writeString(mediaSrc)
        parcel.writeString(thumbnailPath)
        parcel.writeString(author)
        parcel.writeString(duration)
        parcel.writeLong(lastPlayedPosition)
        parcel.writeInt(if (isCached) 1 else 0)
        parcel.writeInt(if (isSelected) 1 else 0)
        parcel.writeLong(fileSize)
    }

    override fun describeContents() = 0
}

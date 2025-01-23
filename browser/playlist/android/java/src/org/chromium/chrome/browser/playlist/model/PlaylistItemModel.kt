/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.model

import android.os.Parcel
import android.os.Parcelable
import com.google.gson.annotations.SerializedName

data class PlaylistItemModel(
    val id: String,
    @SerializedName("playlist_id") var playlistId: String,
    @SerializedName("name") val name: String,
    @SerializedName("page_source") val pageSource: String,
    @SerializedName("media_path") val mediaPath: String,
    @SerializedName("hls_media_path") val hlsMediaPath: String,
    @SerializedName("media_src") val mediaSrc: String,
    @SerializedName("thumbnail_path") val thumbnailPath: String,
    @SerializedName("author") val author: String,
    @SerializedName("duration") val duration: String,
    @SerializedName("last_played_position") var lastPlayedPosition: Long,
    @SerializedName("media_file_bytes") var mediaFileBytes: Long,
    @SerializedName("is_cached") val isCached: Boolean = false,
    @SerializedName("is_selected") var isSelected: Boolean = false
) : Parcelable {
    companion object {
        @JvmField
        @Suppress("unused")
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
        hlsMediaPath = parcel.readString().toString(),
        mediaSrc = parcel.readString().toString(),
        thumbnailPath = parcel.readString().toString(),
        author = parcel.readString().toString(),
        duration = parcel.readString().toString(),
        lastPlayedPosition = parcel.readLong(),
        mediaFileBytes = parcel.readLong(),
        isCached = parcel.readInt() == 1,
        isSelected = parcel.readInt() == 1
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeString(id)
        parcel.writeString(playlistId)
        parcel.writeString(name)
        parcel.writeString(pageSource)
        parcel.writeString(mediaPath)
        parcel.writeString(hlsMediaPath)
        parcel.writeString(mediaSrc)
        parcel.writeString(thumbnailPath)
        parcel.writeString(author)
        parcel.writeString(duration)
        parcel.writeLong(lastPlayedPosition)
        parcel.writeLong(mediaFileBytes)
        parcel.writeInt(if (isCached) 1 else 0)
        parcel.writeInt(if (isSelected) 1 else 0)
    }

    override fun describeContents() = 0
}

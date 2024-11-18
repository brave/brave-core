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
data class LastPlayedPositionModel(
    @PrimaryKey @SerializedName("playlist_item_id") @ColumnInfo(name = "playlist_item_id") var playlistItemId: String,
    @SerializedName("last_played_position") @ColumnInfo(name = "last_played_position") var lastPlayedPosition: Long
) : Parcelable {
    companion object {
        @JvmField
        @Suppress("unused")
        val CREATOR = object : Parcelable.Creator<LastPlayedPositionModel> {
            override fun createFromParcel(parcel: Parcel) = LastPlayedPositionModel(parcel)
            override fun newArray(size: Int) = arrayOfNulls<LastPlayedPositionModel>(size)
        }
    }

    private constructor(parcel: Parcel) : this(
        playlistItemId = parcel.readString().toString(),
        lastPlayedPosition = parcel.readLong(),
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeString(playlistItemId)
        parcel.writeLong(lastPlayedPosition)
    }

    override fun describeContents() = 0
}

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
import org.chromium.chrome.browser.playlist.kotlin.enums.HlsContentStatus
import com.google.gson.annotations.SerializedName

@Entity
data class HlsContentQueueModel(
    @PrimaryKey @SerializedName("playlist_item_id") @ColumnInfo(name = "playlist_item_id") var playlistItemId: String,
    @SerializedName("hls_content_status") @ColumnInfo(name = "hls_content_status") var hlsContentStatus: String = HlsContentStatus.NOT_READY.name
) : Parcelable {
    companion object {
        @JvmField
        @Suppress("unused")
        val CREATOR = object : Parcelable.Creator<HlsContentQueueModel> {
            override fun createFromParcel(parcel: Parcel) = HlsContentQueueModel(parcel)
            override fun newArray(size: Int) = arrayOfNulls<HlsContentQueueModel>(size)
        }
    }

    private constructor(parcel: Parcel) : this(
        playlistItemId = parcel.readString().toString(),
        hlsContentStatus = parcel.readString().toString(),
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeString(playlistItemId)
        parcel.writeString(hlsContentStatus)
    }

    override fun describeContents() = 0
}

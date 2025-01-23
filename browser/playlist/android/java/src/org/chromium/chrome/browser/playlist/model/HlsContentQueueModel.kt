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

data class HlsContentQueueModel(
    @SerializedName("playlist_item_id") var playlistItemId: String,
    @SerializedName("hls_content_status") var hlsContentStatus: String = HlsContentStatus.NOT_READY.name
) : Parcelable {
    enum class HlsContentStatus {
        @Suppress("unused")
        READY,
        NOT_READY
    }

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

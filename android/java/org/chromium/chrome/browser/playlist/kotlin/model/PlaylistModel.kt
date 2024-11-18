/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.model

import android.os.Build
import android.os.Parcel
import android.os.Parcelable
import com.google.gson.annotations.SerializedName

data class PlaylistModel(
    @SerializedName("id") val id: String,
    @SerializedName("name") var name: String,
    @SerializedName("items") var items: List<PlaylistItemModel>
) :
    Parcelable {

    enum class PlaylistOptionsEnum {
        //Playlist button options
        ADD_MEDIA,
        OPEN_PLAYLIST,
        PLAYLIST_SETTINGS,

        // Playlist options
        EDIT_PLAYLIST,
        RENAME_PLAYLIST,
        DELETE_PLAYLIST,

        // Playlist item options
        MOVE_PLAYLIST_ITEM,
        COPY_PLAYLIST_ITEM,
        DELETE_ITEMS_OFFLINE_DATA,
        SHARE_PLAYLIST_ITEM,
        OPEN_IN_NEW_TAB,
        OPEN_IN_PRIVATE_TAB,
        DELETE_PLAYLIST_ITEM,

        // Playlist multiple items options
        MOVE_PLAYLIST_ITEMS,
        COPY_PLAYLIST_ITEMS,

        // Extra options
        NEW_PLAYLIST
    }

    companion object {
        @JvmField
        @Suppress("unused")
        val CREATOR = object : Parcelable.Creator<PlaylistModel> {
            override fun createFromParcel(parcel: Parcel) = PlaylistModel(parcel)
            override fun newArray(size: Int) = arrayOfNulls<PlaylistModel>(size)
        }
    }

    @Suppress("deprecation")
    private constructor(parcel: Parcel) : this(
        id = parcel.readString().toString(),
        name = parcel.readString().toString(),
        arrayListOf<PlaylistItemModel>().apply {
            parcel.readList(this, PlaylistItemModel::class.java.classLoader)
        }
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeString(id)
        parcel.writeString(name)
        if (Build.VERSION.SDK_INT >= 29) {
            parcel.writeParcelableList(items, flags)
        } else {
            parcel.writeList(items)
        }
    }

    override fun describeContents() = 0
}

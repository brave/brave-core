/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.model

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

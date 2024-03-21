/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.model

import android.os.Parcel
import android.os.Parcelable

data class PlaylistOnboardingModel(
    val title: String,
    val message: String,
    val illustration: Int
) : Parcelable {
    companion object {
        @JvmField
        @Suppress("unused")
        val CREATOR = object : Parcelable.Creator<PlaylistOnboardingModel> {
            override fun createFromParcel(parcel: Parcel) = PlaylistOnboardingModel(parcel)
            override fun newArray(size: Int) = arrayOfNulls<PlaylistOnboardingModel>(size)
        }
    }

    private constructor(parcel: Parcel) : this(
        title = parcel.readString().toString(),
        message = parcel.readString().toString(),
        illustration = parcel.readInt()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeString(title)
        parcel.writeString(message)
        parcel.writeInt(illustration)
    }

    override fun describeContents() = 0
}

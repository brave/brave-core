/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.model

data class PlaylistOptionsModel(
    val optionTitle: String,
    val optionIcon: Int,
    val optionType: PlaylistModel.PlaylistOptionsEnum,
    val allPlaylistModels: MutableList<PlaylistModel> = mutableListOf(),
    val playlistModel: PlaylistModel? = null,
    val playlistItemModels: ArrayList<PlaylistItemModel> = arrayListOf() // Used for multiple items
)

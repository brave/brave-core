/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.model

import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.playlist.mojom.Playlist
import org.chromium.playlist.mojom.PlaylistItem

data class PlaylistOptionsModel(
    val optionTitle: String,
    val optionIcon: Int,
    val optionType: PlaylistOptionsEnum,
    val allPlaylists: MutableList<Playlist> = mutableListOf(),
    val playlist: Playlist? = null,
    val playlistItems: ArrayList<PlaylistItem> = arrayListOf() // Used for multiple items
)

/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.enums

import androidx.annotation.Keep

@Keep
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

/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.model

import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.playlist.mojom.PlaylistItem

data class MoveOrCopyModel(
    val playlistOptionsEnum: PlaylistOptionsEnum,
    val fromPlaylistId: String,
    val toPlaylistId: String,
    val playlistItems: List<PlaylistItem>
)

/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.model

import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistBaseActivity
import org.chromium.playlist.mojom.PlaylistItem

data class PlaylistItemOptionModel(
    val optionTitle: String,
    val optionIcon: Int,
    val optionType: PlaylistBaseActivity.PlaylistOptionsEnum,
    val playlistItem: PlaylistItem?,
    val playlistId: String?
)

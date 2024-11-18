/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.listener

import android.view.View
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel

interface PlaylistItemClickListener {
    fun onPlaylistItemClick(position: Int) {}

    fun onPlaylistItemClickInEditMode(count: Int) {}
    fun onPlaylistItemMenuClick(view: View, playlistItemModel: PlaylistItemModel) {}
}

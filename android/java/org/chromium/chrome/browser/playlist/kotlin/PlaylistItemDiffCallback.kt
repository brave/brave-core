/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin

import androidx.recyclerview.widget.DiffUtil
import org.chromium.playlist.mojom.PlaylistItem

class PlaylistItemDiffCallback<T : Any> : DiffUtil.ItemCallback<T>() {
    override fun areItemsTheSame(oldItem: T, newItem: T): Boolean {
        var value = false
        if (oldItem is PlaylistItem && newItem is PlaylistItem) {
            value = oldItem.id == newItem.id
        }
        return value
    }

    override fun areContentsTheSame(oldItem: T, newItem: T): Boolean {
        var value = false
        if (oldItem is PlaylistItem && newItem is PlaylistItem) {
            value =
                (oldItem.id == newItem.id && oldItem.hlsMediaPath.url == newItem.hlsMediaPath.url && oldItem.cached == newItem.cached)
        }
        return value
    }
}

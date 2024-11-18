/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist

import androidx.recyclerview.widget.DiffUtil
import com.brave.playlist.model.PlaylistItemModel

class PlaylistItemDiffCallback<T : Any> : DiffUtil.ItemCallback<T>() {
    override fun areItemsTheSame(oldItem: T, newItem: T): Boolean {
        var value = false
        if (oldItem is PlaylistItemModel && newItem is PlaylistItemModel) {
            value = oldItem.id == newItem.id
        }
        return value
    }

    override fun areContentsTheSame(oldItem: T, newItem: T): Boolean {
        var value = false
        if (oldItem is PlaylistItemModel && newItem is PlaylistItemModel) {
            value =
                (oldItem.id == newItem.id && oldItem.hlsMediaPath == newItem.hlsMediaPath && oldItem.isCached == newItem.isCached)
        }
        return value
    }
}

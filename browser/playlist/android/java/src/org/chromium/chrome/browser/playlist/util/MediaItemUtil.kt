/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.util

import android.os.Bundle
import androidx.core.net.toUri
import androidx.media3.common.MediaItem
import androidx.media3.common.MediaMetadata
import org.chromium.chrome.browser.playlist.model.PlaylistItemModel

object MediaItemUtil {
    @JvmStatic
    fun buildMediaItem(
        playlistItemModel: PlaylistItemModel,
        playlistId: String,
        playlistName: String,
    ): MediaItem {
        val mediaPath = if (playlistItemModel.isCached) {
            if (MediaUtils.isHlsFile(playlistItemModel.mediaPath)) {
                playlistItemModel.hlsMediaPath
            } else {
                playlistItemModel.mediaPath
            }
        } else {
            playlistItemModel.mediaSrc
        }

        val bundle = Bundle()
        bundle.putString(ConstantUtils.PLAYLIST_ID, playlistId)
        val metadata =
            MediaMetadata.Builder()
                .setExtras(bundle)
                .setTitle(playlistName)
                .setArtist(playlistItemModel.name)
                .setArtworkUri(playlistItemModel.thumbnailPath.toUri())
                .build()

        return MediaItem.Builder()
            .setMediaId(playlistItemModel.id)
            .setMediaMetadata(metadata)
            .setRequestMetadata(
                MediaItem.RequestMetadata.Builder().setMediaUri(mediaPath.toUri()).build()
            )
            .setUri(mediaPath.toUri())
            .build()
    }
}

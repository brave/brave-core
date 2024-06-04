/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.util

import android.net.Uri
import android.os.Bundle
import androidx.media3.common.MediaItem
import androidx.media3.common.MediaMetadata

import org.chromium.playlist.mojom.PlaylistItem

object MediaItemUtil {

    @JvmStatic
    fun buildMediaItem(
        playlistItem: PlaylistItem,
        playlistId: String,
        playlistName: String
    ): MediaItem {
        val mediaPath =
            if (playlistItem.cached) {
                if (MediaUtils.isHlsFile(playlistItem.mediaPath.url)) {
                    playlistItem.hlsMediaPath.url
                } else {
                    playlistItem.mediaPath.url
                }
            } else {
                playlistItem.mediaSource.url
            }

        val bundle = Bundle()
        bundle.putString(ConstantUtils.PLAYLIST_ID, playlistId)
        val metadata =
            MediaMetadata.Builder()
                .setExtras(bundle)
                .setTitle(playlistName)
                .setArtist(playlistItem.name)
                .setArtworkUri(Uri.parse(playlistItem.thumbnailPath.url))
                .build()

        return MediaItem.Builder()
            .setMediaId(playlistItem.id)
            .setMediaMetadata(metadata)
            .setRequestMetadata(
                MediaItem.RequestMetadata.Builder().setMediaUri(Uri.parse(mediaPath)).build()
            )
            .setUri(Uri.parse(mediaPath))
            .build()
    }
}

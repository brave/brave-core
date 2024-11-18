/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistClickListener
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import com.bumptech.glide.Glide

class RecentlyPlayedPlaylistAdapter(private val playlistClickListener: PlaylistClickListener?) :
    AbstractRecyclerViewAdapter<PlaylistModel, RecentlyPlayedPlaylistAdapter.RecentlyPlayedPlaylistViewHolder>() {

    class RecentlyPlayedPlaylistViewHolder(
        view: View,
        private val playlistClickListener: PlaylistClickListener?
    ) :
        AbstractViewHolder<PlaylistModel>(view) {
        private val ivPlaylistCover: AppCompatImageView
        private val tvPlaylistName: AppCompatTextView
        private val tvPlaylistItemCount: AppCompatTextView

        init {
            ivPlaylistCover = view.findViewById(R.id.ivPlaylistCover)
            tvPlaylistName = view.findViewById(R.id.tvPlaylistName)
            tvPlaylistItemCount = view.findViewById(R.id.tvPlaylistItemCount)
        }

        override fun onBind(position: Int, model: PlaylistModel) {
            if (model.items.isNotEmpty() && model.items[0].thumbnailPath.isNotEmpty()) {
                Glide.with(itemView.context)
                    .asBitmap()
                    .placeholder(R.drawable.ic_playlist_item_placeholder)
                    .error(R.drawable.ic_playlist_item_placeholder)
                    .load(model.items[0].thumbnailPath)
                    .into(ivPlaylistCover)
            } else {
                ivPlaylistCover.setImageResource(R.drawable.ic_playlist_item_placeholder)
            }
            tvPlaylistName.text =
                if (model.id == ConstantUtils.DEFAULT_PLAYLIST) itemView.context.resources.getString(
                    R.string.playlist_play_later
                ) else model.name
            tvPlaylistItemCount.text =
                itemView.context.getString(R.string.playlist_number_items, model.items.size)
            itemView.setOnClickListener {
                playlistClickListener?.onPlaylistClick(model)
            }
        }
    }

    override fun onCreateViewHolder(
        parent: ViewGroup,
        viewType: Int
    ): RecentlyPlayedPlaylistViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_recently_played_playlist, parent, false)
        return RecentlyPlayedPlaylistViewHolder(view, playlistClickListener)
    }
}

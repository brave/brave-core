/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.adapter.bottomsheet

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import org.chromium.chrome.browser.playlist.R
import org.chromium.chrome.browser.playlist.adapter.recyclerview.AbstractRecyclerViewAdapter
import org.chromium.chrome.browser.playlist.listener.PlaylistItemOptionsListener
import org.chromium.chrome.browser.playlist.model.PlaylistItemOptionModel

class PlaylistItemOptionsBottomSheetAdapter(
    private val playlistItemOptionsListener: PlaylistItemOptionsListener
) :
    AbstractRecyclerViewAdapter<PlaylistItemOptionModel, PlaylistItemOptionsBottomSheetAdapter.PlaylistItemOptionsViewHolder>() {

    class PlaylistItemOptionsViewHolder(
        view: View,
        private val playlistItemOptionsListener: PlaylistItemOptionsListener
    ) :
        AbstractViewHolder<PlaylistItemOptionModel>(view) {
        private val optionView: View
        private val ivOptionIcon: AppCompatImageView
        private val tvOptionTitle: AppCompatTextView

        init {
            optionView = view
            ivOptionIcon = view.findViewById(R.id.ivOptionIcon)
            tvOptionTitle = view.findViewById(R.id.tvOptionTitle)
        }

        override fun onBind(position: Int, model: PlaylistItemOptionModel) {
            ivOptionIcon.setImageResource(model.optionIcon)
            tvOptionTitle.text = model.optionTitle
            optionView.setOnClickListener {
                playlistItemOptionsListener.onPlaylistItemOptionClicked(model)
            }
        }
    }

    override fun onCreateViewHolder(
        parent: ViewGroup,
        viewType: Int
    ): PlaylistItemOptionsViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_playlist_options_bottom_sheet, parent, false)
        return PlaylistItemOptionsViewHolder(view, playlistItemOptionsListener)
    }
}

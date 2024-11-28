/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView

import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.android.material.bottomsheet.BottomSheetDialogFragment
import com.google.android.material.card.MaterialCardView

import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.adapter.bottomsheet.PlaylistOptionsBottomSheetAdapter
import org.chromium.chrome.browser.playlist.kotlin.extension.setTopCornersRounded
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel

class PlaylistOptionsBottomSheet(
    private val playlistOptionsModel: MutableList<PlaylistOptionsModel>,
    private val playlistOptionsListener: PlaylistOptionsListener
) : BottomSheetDialogFragment(), PlaylistOptionsListener {

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? = inflater.inflate(R.layout.bottom_sheet_playlist_options, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val layoutBottomSheet: MaterialCardView = view.findViewById(R.id.layoutBottomSheet)
        layoutBottomSheet.setTopCornersRounded(16)

        val rvBottomSheet: RecyclerView = view.findViewById(R.id.rvBottomSheet)
        rvBottomSheet.layoutManager = LinearLayoutManager(view.context)
        val playlistOptionsBottomSheetAdapter = PlaylistOptionsBottomSheetAdapter(this)
        rvBottomSheet.adapter = playlistOptionsBottomSheetAdapter
        playlistOptionsBottomSheetAdapter.submitList(playlistOptionsModel)

        val behavior = BottomSheetBehavior.from(layoutBottomSheet)
        behavior.state = BottomSheetBehavior.STATE_EXPANDED
        behavior.isDraggable = false
    }

    override fun onPlaylistOptionClicked(playlistOptionsModel: PlaylistOptionsModel) {
        playlistOptionsListener.onPlaylistOptionClicked(playlistOptionsModel)
        dismiss()
    }
}

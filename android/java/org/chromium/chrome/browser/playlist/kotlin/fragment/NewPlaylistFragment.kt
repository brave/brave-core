/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.fragment

import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatEditText
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import org.chromium.chrome.browser.playlist.kotlin.PlaylistViewModel
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.chrome.browser.playlist.kotlin.model.CreatePlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.model.RenamePlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.PLAYLIST_MODEL
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.PLAYLIST_OPTION
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.SHOULD_MOVE_OR_COPY
import org.chromium.chrome.browser.playlist.kotlin.view.PlaylistToolbar

class NewPlaylistFragment : Fragment(R.layout.fragment_new_playlist) {
    private lateinit var mPlaylistViewModel: PlaylistViewModel
    private lateinit var mEtPlaylistName: AppCompatEditText
    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private var mPlaylistModel: PlaylistModel? = null
    private var mPlaylistOptionsEnum: PlaylistOptionsEnum = PlaylistOptionsEnum.NEW_PLAYLIST
    private var mShouldMoveOrCopy: Boolean = false

    @Suppress("deprecation")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let {
            mPlaylistModel = it.getParcelable(PLAYLIST_MODEL)
            mPlaylistOptionsEnum = it.getSerializable(PLAYLIST_OPTION) as PlaylistOptionsEnum
            mShouldMoveOrCopy = it.getBoolean(SHOULD_MOVE_OR_COPY)
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        mPlaylistViewModel = ViewModelProvider(requireActivity())[PlaylistViewModel::class.java]

        mEtPlaylistName = view.findViewById(R.id.etPlaylistName)
        mEtPlaylistName.setText(mPlaylistModel?.name)
        mPlaylistToolbar = view.findViewById(R.id.playlistToolbar)
        mPlaylistToolbar.setToolbarTitle(
            if (mPlaylistOptionsEnum == PlaylistOptionsEnum.NEW_PLAYLIST) getString(
                R.string.playlist_new_text
            ) else getString(R.string.playlist_rename_text)
        )
        mPlaylistToolbar.setActionText(
            if (mPlaylistOptionsEnum == PlaylistOptionsEnum.NEW_PLAYLIST) getString(
                R.string.playlist_create_toolbar_text
            ) else getString(R.string.playlist_rename_text)
        )

        mPlaylistToolbar.setActionButtonClickListener(clickListener = {
            if (mPlaylistOptionsEnum == PlaylistOptionsEnum.NEW_PLAYLIST) {
                if (!mEtPlaylistName.text.isNullOrEmpty()) {
                    mPlaylistViewModel.setCreatePlaylistOption(
                        CreatePlaylistModel(
                            mEtPlaylistName.text.toString(),
                            mShouldMoveOrCopy
                        )
                    )
                    if (activity is AppCompatActivity)
                        (activity as AppCompatActivity).onBackPressedDispatcher.onBackPressed()
                } else {
                    Toast.makeText(
                        requireActivity(),
                        R.string.playlist_empty_playlist_name,
                        Toast.LENGTH_SHORT
                    ).show()
                }
            } else {
                if (!mEtPlaylistName.text.isNullOrEmpty()) {
                    mPlaylistViewModel.setRenamePlaylistOption(
                        RenamePlaylistModel(
                            mPlaylistModel?.id,
                            mEtPlaylistName.text.toString()
                        )
                    )
                    if (activity is AppCompatActivity)
                        (activity as AppCompatActivity).onBackPressedDispatcher.onBackPressed()
                } else {
                    Toast.makeText(
                        requireContext(),
                        R.string.playlist_empty_playlist_name,
                        Toast.LENGTH_SHORT
                    ).show()
                }
            }
        })
        mEtPlaylistName.requestFocus()
    }

    companion object {
        @JvmStatic
        fun newInstance(
            playlistOptionsEnum: PlaylistOptionsEnum,
            playlistModel: PlaylistModel? = null,
            shouldMoveOrCopy: Boolean = false
        ) =
            NewPlaylistFragment().apply {
                arguments = Bundle().apply {
                    putSerializable(PLAYLIST_OPTION, playlistOptionsEnum)
                    putParcelable(PLAYLIST_MODEL, playlistModel)
                    putBoolean(SHOULD_MOVE_OR_COPY, shouldMoveOrCopy)
                }
            }
    }
}

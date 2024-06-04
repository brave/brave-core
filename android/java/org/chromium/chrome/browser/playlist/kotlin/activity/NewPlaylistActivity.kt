/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.widget.Toast
import androidx.appcompat.widget.AppCompatEditText

import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistClickListener
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import org.chromium.chrome.browser.playlist.kotlin.view.PlaylistToolbar
import org.chromium.playlist.mojom.Playlist

class NewPlaylistActivity : PlaylistBaseActivity(), PlaylistClickListener {
    companion object {
        val TAG: String = "NewPlaylistActivity"
    }

    private var mPlaylist: Playlist? = null
    private lateinit var mEtPlaylistName: AppCompatEditText
    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private var mPlaylistOptionsEnum: String? = null
    private var mShouldMoveOrCopy: Boolean = false

    override fun initializeViews() {
        setContentView(R.layout.activity_new_playlist)

        mPlaylistToolbar = findViewById(R.id.playlistToolbar)
        mEtPlaylistName = findViewById(R.id.etPlaylistName)
    }

    override fun onPostCreate() {
        super.onPostCreate()
        mPlaylistId =
            intent.getStringExtra(ConstantUtils.PLAYLIST_ID) ?: ConstantUtils.DEFAULT_PLAYLIST
        mPlaylistOptionsEnum = intent.getStringExtra(ConstantUtils.PLAYLIST_OPTION)
        mShouldMoveOrCopy = intent.getBooleanExtra(ConstantUtils.SHOULD_MOVE_OR_COPY, false)
    }

    override fun finishNativeInitialization() {
        super.finishNativeInitialization()
        fetchPlaylistData()
        mPlaylistToolbar.setActionButtonClickListener(
            clickListener = {
                if (
                    mPlaylistOptionsEnum != null &&
                        mPlaylistOptionsEnum.equals(ConstantUtils.RENAME_OPTION)
                ) {
                    if (!mEtPlaylistName.text.isNullOrEmpty()) {
                        // Rename playlist
                        mPlaylistService?.renamePlaylist(
                            mPlaylist?.id,
                            mEtPlaylistName.text.toString()
                        ) { _ ->
                            finish()
                        }
                    } else {
                        Toast.makeText(
                                this@NewPlaylistActivity,
                                R.string.playlist_empty_playlist_name,
                                Toast.LENGTH_SHORT
                            )
                            .show()
                    }
                } else {
                    if (!mEtPlaylistName.text.isNullOrEmpty()) {
                        // New Playlist
                        val playlist = Playlist()
                        playlist.name = mEtPlaylistName.text.toString()
                        playlist.items = emptyArray()
                        mPlaylistService?.createPlaylist(playlist) { createdPlaylist ->
                            if (
                                PlaylistUtils.moveOrCopyModel.playlistOptionsEnum ==
                                    PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM ||
                                    PlaylistUtils.moveOrCopyModel.playlistOptionsEnum ==
                                        PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS
                            ) {
                                PlaylistUtils.moveOrCopyModel.playlistItems.forEach {
                                    mPlaylistService?.moveItem(
                                        PlaylistUtils.moveOrCopyModel.fromPlaylistId,
                                        createdPlaylist.id,
                                        it.id
                                    )
                                }
                            } else {
                                val playlistItemIds =
                                    Array<String>(
                                        PlaylistUtils.moveOrCopyModel.playlistItems.size
                                    ) {
                                        ""
                                    }
                                for (i in PlaylistUtils.moveOrCopyModel.playlistItems.indices) {
                                    playlistItemIds[i] =
                                        PlaylistUtils.moveOrCopyModel.playlistItems[i].id
                                }
                                mPlaylistService?.copyItemToPlaylist(
                                    playlistItemIds,
                                    createdPlaylist.id
                                )
                            }
                        }
                        finish()
                    } else {
                        Toast.makeText(
                                this@NewPlaylistActivity,
                                R.string.playlist_empty_playlist_name,
                                Toast.LENGTH_SHORT
                            )
                            .show()
                    }
                }
            }
        )
    }

    private fun fetchPlaylistData() {
        mPlaylistService?.getPlaylist(mPlaylistId) { playlist ->
            mPlaylist = playlist

            mEtPlaylistName.setText(playlist?.name)
            mEtPlaylistName.requestFocus()

            var toolbarTitle = getString(R.string.playlist_new_text)
            var toolbarActionText = getString(R.string.playlist_create_toolbar_text)
            if (
                mPlaylistOptionsEnum != null &&
                    mPlaylistOptionsEnum.equals(ConstantUtils.RENAME_OPTION)
            ) {
                toolbarTitle = getString(R.string.playlist_rename_text)
                toolbarActionText = getString(R.string.playlist_rename_text)
            }
            mPlaylistToolbar.setToolbarTitle(toolbarTitle)
            mPlaylistToolbar.setActionText(toolbarActionText)
        }
    }
}

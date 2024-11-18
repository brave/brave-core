/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.fragment

import android.os.Bundle
import android.util.Log
import android.view.View
import androidx.appcompat.widget.AppCompatButton
import androidx.appcompat.widget.AppCompatTextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.chromium.chrome.browser.playlist.kotlin.PlaylistViewModel
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview.PlaylistAdapter
import org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview.RecentlyPlayedPlaylistAdapter
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistClickListener
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.DEFAULT_PLAYLIST
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.recentlyPlayedPlaylist
import org.chromium.chrome.browser.playlist.kotlin.view.PlaylistToolbar
import com.google.gson.GsonBuilder
import com.google.gson.reflect.TypeToken
import java.util.LinkedList

class AllPlaylistFragment : Fragment(R.layout.fragment_all_playlist), PlaylistOptionsListener,
    PlaylistClickListener {
    companion object {
        val TAG: String = "Playlist/" + this::class.java.simpleName
    }

    private lateinit var mPlaylistViewModel: PlaylistViewModel

    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private lateinit var mBtAddNewPlaylist: AppCompatButton
    private lateinit var mRvRecentlyPlayed: RecyclerView
    private lateinit var mRvPlaylist: RecyclerView
    private lateinit var mTvRecentlyPlayed: AppCompatTextView

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        mPlaylistViewModel = ViewModelProvider(requireActivity())[PlaylistViewModel::class.java]

        mPlaylistToolbar = view.findViewById(R.id.playlistToolbar)

        mBtAddNewPlaylist = view.findViewById(R.id.btAddNewPlaylist)
        mBtAddNewPlaylist.setOnClickListener {
            val newPlaylistFragment = NewPlaylistFragment.newInstance(
                PlaylistModel.PlaylistOptionsEnum.NEW_PLAYLIST
            )
            parentFragmentManager
                .beginTransaction()
                .replace(android.R.id.content, newPlaylistFragment)
                .addToBackStack(AllPlaylistFragment::class.simpleName)
                .commit()
        }
        mRvRecentlyPlayed = view.findViewById(R.id.rvRecentlyPlayed)
        mRvPlaylist = view.findViewById(R.id.rvPlaylists)

        mTvRecentlyPlayed = view.findViewById(R.id.tvRecentlyPlayed)

        mPlaylistViewModel.fetchPlaylistData(ConstantUtils.ALL_PLAYLIST)

        mPlaylistViewModel.allPlaylistData.observe(viewLifecycleOwner) { allPlaylistData ->
            Log.e(TAG, allPlaylistData.toString())
            val allPlaylistList = mutableListOf<PlaylistModel>()

            var defaultPlaylistModel: PlaylistModel? = null
            for (allPlaylistModel in allPlaylistData) {
                val playlistModel = PlaylistModel(
                    allPlaylistModel.id,
                    allPlaylistModel.name,
                    allPlaylistModel.items
                )

                if (playlistModel.id == DEFAULT_PLAYLIST) {
                    defaultPlaylistModel = playlistModel
                } else {
                    allPlaylistList.add(
                        playlistModel
                    )
                }
            }
            defaultPlaylistModel?.let { allPlaylistList.add(0, it) }

            val recentPlaylistJson =
                PlaylistPreferenceUtils.defaultPrefs(requireContext()).recentlyPlayedPlaylist
            if (!recentPlaylistJson.isNullOrEmpty()) {
                val recentPlaylist = LinkedList<PlaylistModel>()
                val recentPlaylistIds: LinkedList<String> = GsonBuilder().create().fromJson(
                    recentPlaylistJson,
                    TypeToken.getParameterized(LinkedList::class.java, String::class.java).type
                )
                if (recentPlaylistIds.size > 0) {
                    recentPlaylistIds.forEach ids@{
                        allPlaylistList.forEach models@{ model ->
                            if (model.id == it && model.items.isNotEmpty()) {
                                recentPlaylist.add(model)
                                return@models
                            }
                        }
                    }
                }
                mRvRecentlyPlayed.layoutManager =
                    LinearLayoutManager(requireContext(), LinearLayoutManager.HORIZONTAL, false)
                val recentlyPlayedPlaylistAdapter = RecentlyPlayedPlaylistAdapter(this)
                mRvRecentlyPlayed.adapter = recentlyPlayedPlaylistAdapter
                recentlyPlayedPlaylistAdapter.submitList(recentPlaylist)
                mRvRecentlyPlayed.visibility =
                    if (recentPlaylist.isNotEmpty()) View.VISIBLE else View.GONE
                mTvRecentlyPlayed.visibility =
                    if (recentPlaylist.isNotEmpty()) View.VISIBLE else View.GONE
            }

            mRvPlaylist.layoutManager = LinearLayoutManager(requireContext())
            val playlistAdapter = PlaylistAdapter(this)
            mRvPlaylist.adapter = playlistAdapter
            playlistAdapter.submitList(allPlaylistList)
        }
    }

    override fun onPlaylistOptionClicked(playlistOptionsModel: PlaylistOptionsModel) {
        mPlaylistViewModel.setAllPlaylistOption(playlistOptionsModel)
    }

    override fun onPlaylistClick(playlistModel: PlaylistModel) {
        mPlaylistViewModel.setPlaylistToOpen(playlistModel.id)
    }
}

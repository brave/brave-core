/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.os.Bundle
import android.widget.ScrollView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatButton
import androidx.viewpager2.widget.ViewPager2
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.adapter.PlaylistOnboardingFragmentStateAdapter
import org.chromium.chrome.browser.playlist.kotlin.extension.afterMeasured
import org.chromium.chrome.browser.playlist.kotlin.extension.showOnboardingGradientBg
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayoutMediator
import org.chromium.chrome.browser.init.AsyncInitializationActivity
import org.chromium.mojo.bindings.ConnectionErrorHandler
import org.chromium.mojo.system.MojoException
import org.chromium.playlist.mojom.PlaylistService
import org.chromium.chrome.browser.flags.ChromeFeatureList
import org.chromium.chrome.browser.init.ActivityProfileProvider
import org.chromium.base.BraveFeatureList
import org.chromium.base.BravePreferenceKeys
import org.chromium.base.Log
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate

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
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
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

class AllPlaylistActivity : AsyncInitializationActivity, ConnectionErrorHandler, PlaylistServiceObserverImplDelegate {
    companion object {
        val TAG: String = this::class.java.simpleName
    }
    private lateinit var mPlaylistService: PlaylistService
    private lateinit var mPlaylistServiceObserver: PlaylistServiceObserverImpl

    private lateinit var mPlaylistViewModel: PlaylistViewModel

    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private lateinit var mBtAddNewPlaylist: AppCompatButton
    private lateinit var mRvRecentlyPlayed: RecyclerView
    private lateinit var mRvPlaylist: RecyclerView
    private lateinit var mTvRecentlyPlayed: AppCompatTextView

    override fun onConnectionError(mojoException : MojoException) {
        mPlaylistService?.close()
        // mPlaylistService = null
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)) {
            initPlaylistService()
        }
    }

    fun initPlaylistService() {
        // if (mPlaylistService != null) {
        //     mPlaylistService = null;
        // }
        mPlaylistService =
                PlaylistServiceFactoryAndroid.getInstance()
                        .getPlaylistService(
                                getProfileProviderSupplier().get().getOriginalProfile(), this)
        addPlaylistObserver()
    }

    private fun addPlaylistObserver() {
        mPlaylistServiceObserver = PlaylistServiceObserverImpl(this)
        mPlaylistService?.addObserver(mPlaylistServiceObserver)
    }

    private fun initializeViews() {
        setContentView(R.layout.fragment_all_playlist)

        mPlaylistToolbar = view.findViewById(R.id.playlistToolbar)

        mBtAddNewPlaylist = view.findViewById(R.id.btAddNewPlaylist)
        mBtAddNewPlaylist.setOnClickListener {
            // val newPlaylistFragment = NewPlaylistFragment.newInstance(
            //     PlaylistOptionsEnum.NEW_PLAYLIST
            // )
            // parentFragmentManager
            //     .beginTransaction()
            //     .replace(android.R.id.content, newPlaylistFragment)
            //     .addToBackStack(AllPlaylistFragment::class.simpleName)
            //     .commit()
        }
        mRvRecentlyPlayed = view.findViewById(R.id.rvRecentlyPlayed)
        mRvPlaylist = view.findViewById(R.id.rvPlaylists)

        mTvRecentlyPlayed = view.findViewById(R.id.tvRecentlyPlayed)

        // mPlaylistViewModel.fetchPlaylistData(ConstantUtils.ALL_PLAYLIST)

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

    override fun triggerLayoutInflation() {
        initializeViews()
        onInitialLayoutInflationComplete()
    }

    override fun finishNativeInitialization() {
        super.finishNativeInitialization()
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            initPlaylistService();
        }
    }

    override fun onDestroy() {
        // if (mPlaylistService != null) {
            mPlaylistService?.close();
            // mPlaylistService = null;
        // }
        // if (mPlaylistServiceObserver != null) {
            mPlaylistServiceObserver?.close();
            mPlaylistServiceObserver?.destroy();
            // mPlaylistServiceObserver = null;
        // }
        super.onDestroy();
    }

    override fun shouldStartGpuProcess() : Boolean {
        return true;
    }

    override fun createProfileProvider() : OneshotSupplier<ProfileProvider> {
        return ActivityProfileProvider(getLifecycleDispatcher());
    }

    // override fun onPlaylistOptionClicked(playlistOptionsModel: PlaylistOptionsModel) {
    //     mPlaylistViewModel.setAllPlaylistOption(playlistOptionsModel)
    // }

    // override fun onPlaylistClick(playlistModel: PlaylistModel) {
    //     mPlaylistViewModel.setPlaylistToOpen(playlistModel.id)
    // }
}

/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.os.Bundle
import android.content.Intent
import android.widget.ScrollView
import androidx.appcompat.app.AppCompatActivity
import androidx.viewpager2.widget.ViewPager2
import org.chromium.chrome.browser.playlist.kotlin.adapter.PlaylistOnboardingFragmentStateAdapter
import org.chromium.chrome.browser.playlist.kotlin.extension.afterMeasured
import org.chromium.chrome.browser.playlist.kotlin.extension.showOnboardingGradientBg
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayoutMediator
import org.chromium.chrome.browser.init.AsyncInitializationActivity
import org.chromium.mojo.bindings.ConnectionErrorHandler
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.mojo.system.MojoException
import org.chromium.playlist.mojom.PlaylistService
import org.chromium.chrome.browser.flags.ChromeFeatureList
import org.chromium.chrome.browser.init.ActivityProfileProvider
import org.chromium.base.BraveFeatureList
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.base.BravePreferenceKeys
import org.chromium.base.Log
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate
import org.chromium.playlist.mojom.Playlist
import org.chromium.playlist.mojom.PlaylistItem
import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistBaseActivity

import android.view.View
import androidx.appcompat.widget.AppCompatButton
import androidx.appcompat.widget.AppCompatTextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.chromium.chrome.browser.playlist.kotlin.activity.NewPlaylistActivity
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

class AllPlaylistActivity : PlaylistBaseActivity(), PlaylistClickListener {
    companion object {
        val TAG: String = "AllPlaylistActivity"
    }

    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private lateinit var mBtAddNewPlaylist: AppCompatButton
    private lateinit var mRvRecentlyPlayed: RecyclerView
    private lateinit var mRvPlaylist: RecyclerView
    private lateinit var mTvRecentlyPlayed: AppCompatTextView

    override fun initializeViews() {
        setContentView(R.layout.fragment_all_playlist)

        mPlaylistToolbar = findViewById(R.id.playlistToolbar)

        mBtAddNewPlaylist = findViewById(R.id.btAddNewPlaylist)
        mBtAddNewPlaylist.setOnClickListener {
            val newActivityIntent = Intent(this@AllPlaylistActivity, NewPlaylistActivity::class.java);
            startActivity(newActivityIntent);
        }
        mRvRecentlyPlayed = findViewById(R.id.rvRecentlyPlayed)
        mRvPlaylist = findViewById(R.id.rvPlaylists)
        mTvRecentlyPlayed = findViewById(R.id.tvRecentlyPlayed)
    }
    
    override fun onResumeWithNative() {
        super.onResumeWithNative();
        mPlaylistService?.getAllPlaylists {
                playlists -> 
                    Log.e(TAG, playlists.toString())
                    val allPlaylistList = mutableListOf<Playlist>()

                    var defaultPlaylist: Playlist? = null
                    for (playlist in playlists) {

                        if (playlist.id == DEFAULT_PLAYLIST) {
                            defaultPlaylist = playlist
                        } else {
                            allPlaylistList.add(
                                playlist
                            )
                        }
                    }
                    defaultPlaylist?.let { allPlaylistList.add(0, it) }
                    mRvPlaylist.layoutManager = LinearLayoutManager(this@AllPlaylistActivity)
                    val playlistAdapter = PlaylistAdapter(this@AllPlaylistActivity)
                    mRvPlaylist.adapter = playlistAdapter
                    playlistAdapter.submitList(allPlaylistList)

                    val recentPlaylistJson = PlaylistPreferenceUtils.defaultPrefs(this@AllPlaylistActivity).recentlyPlayedPlaylist
                    if (!recentPlaylistJson.isNullOrEmpty()) {
                        val recentPlaylist = LinkedList<Playlist>()
                        val recentPlaylistIds: LinkedList<String> = GsonBuilder().create().fromJson(
                            recentPlaylistJson,
                            TypeToken.getParameterized(LinkedList::class.java, String::class.java).type
                        )
                        if (recentPlaylistIds.size > 0) {
                            recentPlaylistIds.forEach ids@{
                                allPlaylistList.forEach playlists@{ playlist ->
                                    if (playlist.id == it && playlist.items.isNotEmpty()) {
                                        recentPlaylist.add(playlist)
                                        return@playlists
                                    }
                                }
                            }
                        }
                        mRvRecentlyPlayed.layoutManager =
                            LinearLayoutManager(this@AllPlaylistActivity, LinearLayoutManager.HORIZONTAL, false)
                        val recentlyPlayedPlaylistAdapter = RecentlyPlayedPlaylistAdapter(this@AllPlaylistActivity)
                        mRvRecentlyPlayed.adapter = recentlyPlayedPlaylistAdapter
                        recentlyPlayedPlaylistAdapter.submitList(recentPlaylist)
                        mRvRecentlyPlayed.visibility =
                            if (recentPlaylist.isNotEmpty()) View.VISIBLE else View.GONE
                        mTvRecentlyPlayed.visibility =
                            if (recentPlaylist.isNotEmpty()) View.VISIBLE else View.GONE
                    }
                };
    }

    override fun onPlaylistClick(playlist: Playlist) {
        val playlistActivityIntent = Intent(this@AllPlaylistActivity, PlaylistActivity::class.java)
        playlistActivityIntent.putExtra(ConstantUtils.PLAYLIST_ID, playlist.id)
        startActivity(playlistActivityIntent)
    }
}

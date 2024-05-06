/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.os.Bundle
import android.os.Build
import android.content.Intent
import android.widget.Toast
import android.widget.ScrollView
import androidx.appcompat.widget.AppCompatEditText
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
import java.io.Serializable

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
import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistBaseActivity

class NewPlaylistActivity : PlaylistBaseActivity(), PlaylistClickListener {
    companion object {
        val TAG: String = "NewPlaylistActivity"
    }

    // private lateinit var mPlaylistViewModel: PlaylistViewModel
    private var mPlaylist: Playlist? = null
    private lateinit var mEtPlaylistName: AppCompatEditText
    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private var mPlaylistOptionsEnum: String? = null
    private var mShouldMoveOrCopy: Boolean = false

    override fun initializeViews() {
        setContentView(R.layout.fragment_new_playlist)

        mPlaylistToolbar = findViewById(R.id.playlistToolbar)
        mEtPlaylistName = findViewById(R.id.etPlaylistName)
    }

    override fun onPostCreate() {
        super.onPostCreate()
        mPlaylistId = intent.getStringExtra(ConstantUtils.PLAYLIST_ID)?:ConstantUtils.DEFAULT_PLAYLIST
        mPlaylistOptionsEnum = intent.getStringExtra(ConstantUtils.PLAYLIST_OPTION)
        mShouldMoveOrCopy = intent.getBooleanExtra(ConstantUtils.SHOULD_MOVE_OR_COPY, false)
    }

    override fun finishNativeInitialization() {
        super.finishNativeInitialization()
        fetchPlaylistData()
        mPlaylistToolbar.setActionButtonClickListener(clickListener = {
            if (mPlaylistOptionsEnum != null && mPlaylistOptionsEnum.equals(ConstantUtils.RENAME_OPTION)) {
                if (!mEtPlaylistName.text.isNullOrEmpty()) {
                    // Rename playlist
                    mPlaylistService?.renamePlaylist(mPlaylist?.id, mEtPlaylistName.text.toString()) {
                        _ -> 
                    }
                    finish()
                } else {
                    Toast.makeText(
                        this@NewPlaylistActivity,
                        R.string.playlist_empty_playlist_name,
                        Toast.LENGTH_SHORT
                    ).show()
                }
            } else {
                if (!mEtPlaylistName.text.isNullOrEmpty()) {
                    // New Playlist
                    val playlist = Playlist()
                    playlist.name = mEtPlaylistName.text.toString()
                    playlist.items = emptyArray()
                    mPlaylistService?.createPlaylist(playlist) {
                        createdPlaylist -> 
                        if (PlaylistUtils.moveOrCopyModel.playlistOptionsEnum
                                            == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM
                                    || PlaylistUtils.moveOrCopyModel.playlistOptionsEnum
                                            == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS) {
                            PlaylistUtils.moveOrCopyModel.playlistItems.forEach {
                                    mPlaylistService?.moveItem(PlaylistUtils.moveOrCopyModel.fromPlaylistId,
                                                            createdPlaylist.id,
                                                            it.id);
                                }
                            } else {
                                val playlistItemIds = Array<String>(PlaylistUtils.moveOrCopyModel.playlistItems.size) { "" }
                                for (i in PlaylistUtils.moveOrCopyModel.playlistItems.indices) {
                                    playlistItemIds[i] = PlaylistUtils.moveOrCopyModel.playlistItems[i].id
                                }
                                mPlaylistService?.copyItemToPlaylist(playlistItemIds, createdPlaylist.id)
                            }
                    }
                    finish()
                } else {
                    Toast.makeText(
                        this@NewPlaylistActivity,
                        R.string.playlist_empty_playlist_name,
                        Toast.LENGTH_SHORT
                    ).show()
                }
            }
        })
    }

    private fun fetchPlaylistData() {
        mPlaylistService?.getPlaylist(mPlaylistId) {
                playlist -> 
                    Log.e(TAG, playlist.toString())
                    mPlaylist = playlist

                    mEtPlaylistName.setText(playlist?.name)
                    mEtPlaylistName.requestFocus()

                    var toolbarTitle = getString(R.string.playlist_new_text)
                    var toolbarActionText = getString(R.string.playlist_create_toolbar_text)
                    if (mPlaylistOptionsEnum != null && mPlaylistOptionsEnum.equals(ConstantUtils.RENAME_OPTION)) {
                        toolbarTitle = getString(R.string.playlist_rename_text)
                        toolbarActionText = getString(R.string.playlist_rename_text)
                    }
                    mPlaylistToolbar.setToolbarTitle(toolbarTitle)
                    mPlaylistToolbar.setActionText(toolbarActionText)
                };
    }
}

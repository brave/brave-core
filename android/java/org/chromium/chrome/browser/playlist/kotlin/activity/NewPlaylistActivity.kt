/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.os.Bundle
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

    private lateinit var mPlaylistViewModel: PlaylistViewModel
    private lateinit var mEtPlaylistName: AppCompatEditText
    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private var mPlaylistModel: PlaylistModel? = null
    private var mPlaylistOptionsEnum: PlaylistOptionsEnum = PlaylistOptionsEnum.NEW_PLAYLIST
    private var mShouldMoveOrCopy: Boolean = false

    override fun initializeViews() {
        setContentView(R.layout.fragment_new_playlist)

        mPlaylistToolbar = findViewById(R.id.playlistToolbar)
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

        mEtPlaylistName = findViewById(R.id.etPlaylistName)
        mEtPlaylistName.setText(mPlaylistModel?.name)
        mEtPlaylistName.requestFocus()
    }

    override fun finishNativeInitialization() {
        super.finishNativeInitialization()
        mPlaylistToolbar.setActionButtonClickListener(clickListener = {
            if (mPlaylistOptionsEnum == PlaylistOptionsEnum.NEW_PLAYLIST) {
                if (!mEtPlaylistName.text.isNullOrEmpty()) {
                    // mPlaylistViewModel.setCreatePlaylistOption(
                    //     CreatePlaylistModel(
                    //         mEtPlaylistName.text.toString(),
                    //         mShouldMoveOrCopy
                    //     )
                    // )
                    // New Playlist
                    val playlist = Playlist()
                    playlist.name = mEtPlaylistName.text.toString()
                    playlist.items = emptyArray()
                    mPlaylistService?.createPlaylist(playlist) {
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
                    // Rename playlist
                    // mPlaylistViewModel.setRenamePlaylistOption(
                    //     RenamePlaylistModel(
                    //         mPlaylistModel?.id,
                    //         mEtPlaylistName.text.toString()
                    //     )
                    // )
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
}

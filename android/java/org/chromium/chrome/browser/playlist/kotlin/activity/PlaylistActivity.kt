/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.os.Bundle
import android.content.ComponentName
import android.text.TextUtils
import android.widget.Toast
import android.text.format.Formatter
import androidx.appcompat.widget.AppCompatButton
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import androidx.core.widget.ContentLoadingProgressBar
import androidx.appcompat.widget.LinearLayoutCompat
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemOptionModel
import android.content.Intent
import org.chromium.playlist.mojom.PlaylistEvent
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.util.MenuUtils
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.util.TabUtils
import org.chromium.chrome.browser.playlist.kotlin.playback_service.VideoPlaybackService
import org.chromium.chrome.browser.init.AsyncInitializationActivity
import org.chromium.mojo.bindings.ConnectionErrorHandler
import androidx.recyclerview.widget.ItemTouchHelper
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel
import org.chromium.base.supplier.OneshotSupplier
import org.chromium.chrome.browser.playlist.kotlin.listener.ItemInteractionListener
import androidx.core.content.ContextCompat
import org.chromium.chrome.browser.playlist.kotlin.model.HlsContentProgressModel
import org.chromium.mojo.system.MojoException
import org.chromium.playlist.mojom.PlaylistService
import org.chromium.chrome.browser.flags.ChromeFeatureList
import org.chromium.chrome.browser.init.ActivityProfileProvider
import org.chromium.base.BraveFeatureList
import org.chromium.chrome.browser.playlist.kotlin.model.MoveOrCopyModel
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.base.BravePreferenceKeys
import androidx.media3.common.MediaItem
import androidx.media3.common.util.UnstableApi
import org.chromium.chrome.browser.playlist.kotlin.view.PlaylistToolbar
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet
import androidx.media3.session.MediaBrowser
import androidx.media3.session.SessionToken
import org.chromium.base.Log
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import org.chromium.chrome.browser.playlist.kotlin.listener.StartDragListener
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemClickListener
import org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview.PlaylistItemAdapter
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.getLatestPlaylistItem
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.recentlyPlayedPlaylist
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.rememberListPlaybackPosition
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistItemGestureHelper
import org.chromium.playlist.mojom.Playlist
import org.chromium.playlist.mojom.PlaylistItem
import java.util.LinkedList
import com.google.gson.GsonBuilder
import com.google.gson.reflect.TypeToken

import android.view.View
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistBaseActivity
import com.google.common.util.concurrent.ListenableFuture
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import org.chromium.chrome.browser.playlist.kotlin.util.MediaItemUtil
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener

class PlaylistActivity : PlaylistBaseActivity(), PlaylistItemClickListener,StartDragListener, ItemInteractionListener, PlaylistOptionsListener  {
    companion object {
        val TAG: String = "PlaylistActivity"
    }

    private lateinit var mPlaylist: Playlist

    private var mPlaylistItemAdapter: PlaylistItemAdapter? = null

    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private lateinit var mRvPlaylist: RecyclerView
    private lateinit var mTvTotalMediaCount: AppCompatTextView
    private lateinit var mTvPlaylistName: AppCompatTextView
    private lateinit var mLayoutPlayMedia: LinearLayoutCompat
    private lateinit var mIvPlaylistOptions: AppCompatImageView
    private lateinit var mProgressBar: ContentLoadingProgressBar
    private lateinit var mLayoutShuffleMedia: LinearLayoutCompat
    private lateinit var mIvPlaylistCover: AppCompatImageView
    private lateinit var mTvPlaylistTotalSize: AppCompatTextView
    private lateinit var mItemTouchHelper: ItemTouchHelper

    private lateinit var mEmptyView: View
    private lateinit var mPlaylistView: View
    private var isFirstRun:Boolean = true

    private lateinit var mBrowserFuture: ListenableFuture<MediaBrowser>
    private val mMediaBrowser: MediaBrowser?
        get() = if (mBrowserFuture.isDone) mBrowserFuture.get() else null

    private fun initializeBrowser() {
        mBrowserFuture =
            MediaBrowser.Builder(
                this@PlaylistActivity,
                SessionToken(
                    this@PlaylistActivity,
                    ComponentName(this@PlaylistActivity, VideoPlaybackService::class.java)
                )
            )
                .buildAsync()
        mBrowserFuture.addListener({ }, ContextCompat.getMainExecutor(this@PlaylistActivity))
    }

    private fun releaseBrowser() {
        MediaBrowser.releaseFuture(mBrowserFuture)
    }

    override fun onDestroy() {
        super.onDestroy()
        releaseBrowser()
    }

    override fun initializeViews() {
        setContentView(R.layout.fragment_playlist)

        mEmptyView = findViewById(R.id.empty_view)
        mPlaylistView = findViewById(R.id.playlist_view)

        mPlaylistToolbar = findViewById(R.id.playlistToolbar)

        mRvPlaylist = findViewById(R.id.rvPlaylists)
        mTvTotalMediaCount = findViewById(R.id.tvTotalMediaCount)
        mTvPlaylistName = findViewById(R.id.tvPlaylistName)
        mLayoutPlayMedia = findViewById(R.id.layoutPlayMedia)
        mLayoutShuffleMedia = findViewById(R.id.layoutShuffleMedia)
        mIvPlaylistOptions = findViewById(R.id.ivPlaylistOptions)
        mIvPlaylistCover = findViewById(R.id.ivPlaylistCover)
        mTvPlaylistTotalSize = findViewById(R.id.tvPlaylistTotalSize)
        mProgressBar = findViewById(R.id.progressBar)

        mPlaylistToolbar.setExitEditModeClickListener {
            mPlaylistItemAdapter?.setEditMode(false)
            mPlaylistToolbar.updateSelectedItems(0)
            mPlaylistToolbar.enableEditMode(false)
            mIvPlaylistOptions.visibility = View.VISIBLE
            //Reorder list
            mPlaylistItemAdapter?.currentList
                ?.let { playlistItems ->
                    playlistItems.forEachIndexed { index, playlistItem ->
                        mPlaylistService?.reorderItemFromPlaylist(mPlaylistId,playlistItem.id, index.toShort()) {}
                    }
                    // mPlaylistViewModel.reorderPlaylistItems(playlistItems)
                    // VideoPlaybackService.reorderPlaylistItemModel(playlistItems)
                }
        }
        mPlaylistToolbar.setMoveClickListener { actionView ->
            mPlaylistItemAdapter?.getSelectedItems()?.let {
                if (it.size > 0) {
                    MenuUtils.showMoveOrCopyMenu(
                        actionView, supportFragmentManager, it, this@PlaylistActivity
                    )
                } else {
                    Toast.makeText(
                        this@PlaylistActivity,
                        getString(R.string.playlist_please_select_items),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }
        mPlaylistToolbar.setDeleteClickListener {
            mPlaylistItemAdapter?.getSelectedItems()?.let {
                if (it.size > 0) {
                    for (selectedItem in it) {
                        stopVideoPlayerOnDelete(selectedItem)
                    }
                    // mPlaylistViewModel.setDeletePlaylistItems(
                    //     PlaylistModel(
                    //         mPlaylistModel.id, mPlaylistModel.name, it
                    //     )
                    // )
                    deletePlaylistItems(it)
                    mPlaylistItemAdapter?.setEditMode(false)
                    mPlaylistToolbar.enableEditMode(false)
                    mIvPlaylistOptions.visibility = View.VISIBLE
                } else {
                    Toast.makeText(
                        this@PlaylistActivity,
                        getString(R.string.playlist_please_select_items),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }

        findViewById<AppCompatButton>(R.id.btBrowseForMedia).setOnClickListener {
            finish()
        }
        mIvPlaylistOptions.setOnClickListener {
            if (mPlaylistId == ConstantUtils.DEFAULT_PLAYLIST) {
                mPlaylistItemAdapter?.setEditMode(true)
                mPlaylistToolbar.enableEditMode(true)
                mIvPlaylistOptions.visibility = View.GONE
            } else {
                MenuUtils.showPlaylistMenu(
                    this@PlaylistActivity,
                    supportFragmentManager,
                    mPlaylist,
                    this@PlaylistActivity,
                    mPlaylist.id == ConstantUtils.DEFAULT_PLAYLIST
                )
            }
        }

        mLayoutPlayMedia.setOnClickListener {
                    if (PlaylistPreferenceUtils.defaultPrefs(this@PlaylistActivity).rememberListPlaybackPosition && !TextUtils.isEmpty(
                            PlaylistPreferenceUtils.defaultPrefs(this@PlaylistActivity)
                                .getLatestPlaylistItem(mPlaylistId)
                        )
                    ) {
                        mPlaylist.items.forEachIndexed { index, playlistToOpen ->
                            if (playlistToOpen.id == PlaylistPreferenceUtils.defaultPrefs(
                                    this@PlaylistActivity
                                ).getLatestPlaylistItem(mPlaylist.id)
                            ) {
                                openPlaylistPlayer(false, index)
                                return@forEachIndexed
                            }
                        }
                    } else {
                        openPlaylistPlayer(false, 0)
                    }
                }

        mLayoutShuffleMedia.setOnClickListener {
            openPlaylistPlayer(true, (0 until mPlaylist.items.size).shuffled().first())
        }
    }

    override fun onResumeWithNative() {
        super.onResumeWithNative();

        Log.e(TAG, "onResumeWithNative")

        mPlaylistId = intent.getStringExtra(ConstantUtils.PLAYLIST_ID)?:ConstantUtils.DEFAULT_PLAYLIST
        initializeBrowser()

        VideoPlaybackService.currentPlayingItem.observe(this@PlaylistActivity) { currentPlayingItemId ->
            if (!currentPlayingItemId.isNullOrEmpty()) {
                mPlaylistItemAdapter?.updatePlayingStatus(currentPlayingItemId)
            }
        }

        fetchPlaylistData()
    }

    private fun fetchPlaylistData() {
        mProgressBar.visibility = View.VISIBLE
        mRvPlaylist.visibility = View.GONE
        mPlaylistService?.getPlaylist(mPlaylistId) {
                playlist -> 
                    Log.e(TAG, playlist.toString())
                    mPlaylist = playlist

                    mIvPlaylistOptions.setImageResource(if (mPlaylistId == ConstantUtils.DEFAULT_PLAYLIST) R.drawable.ic_edit_playlist else R.drawable.ic_options_toolbar_playlist)

                    if (playlist.items.isNotEmpty()) {
                        var totalFileSize = 0L
                        playlist.items.forEach {
                            if (it.cached) {
                                totalFileSize += it.mediaFileBytes
                            }
                        }
                        if (totalFileSize > 0) {
                            mTvPlaylistTotalSize.text =
                                Formatter.formatShortFileSize(this@PlaylistActivity, totalFileSize)
                        }

                        Glide.with(this@PlaylistActivity).load(playlist.items[0].thumbnailPath.url)
                        .placeholder(R.drawable.ic_playlist_placeholder)
                        .error(R.drawable.ic_playlist_placeholder).into(mIvPlaylistCover)

                        mTvTotalMediaCount.text = getString(R.string.playlist_number_items, mPlaylist.items.size)
                        mTvPlaylistName.text = if (mPlaylist.id == ConstantUtils.DEFAULT_PLAYLIST) resources.getString(R.string.playlist_play_later) else mPlaylist.name

                        mPlaylistItemAdapter = PlaylistItemAdapter(
                            this@PlaylistActivity, this@PlaylistActivity
                        )
                        mPlaylistItemAdapter?.submitList(playlist.items.toMutableList())
                        mPlaylistItemAdapter?.let {
                            val callback = PlaylistItemGestureHelper(
                                this@PlaylistActivity, mRvPlaylist, it, this@PlaylistActivity
                            )
                            mItemTouchHelper = ItemTouchHelper(callback)
                            mItemTouchHelper.attachToRecyclerView(mRvPlaylist)
                        }
                        mRvPlaylist.adapter = mPlaylistItemAdapter
                        mPlaylistItemAdapter?.setEditMode(false)
                        mPlaylistToolbar.enableEditMode(false)
                        mIvPlaylistOptions.visibility = View.VISIBLE
                        mPlaylistView.visibility = View.VISIBLE
                        mEmptyView.visibility = View.GONE

                        mProgressBar.visibility = View.GONE
                        mRvPlaylist.visibility = View.VISIBLE
                    } else {
                        setEmptyView()
                    }
                };
    }

    private fun setEmptyView() {
        mIvPlaylistCover.setImageResource(R.drawable.ic_playlist_placeholder)
        mEmptyView.visibility = View.VISIBLE
        mPlaylistView.visibility = View.GONE
    }

    private fun stopVideoPlayerOnDelete(selectedPlaylistItem: PlaylistItem) {
        mMediaBrowser?.currentMediaItem?.mediaId?.let {
            if (it == selectedPlaylistItem.id) {
                mMediaBrowser?.stop()
            }
        }
    }

    private fun openPlaylistPlayer(isShuffle: Boolean, position: Int) {
        val browser = this.mMediaBrowser ?: return

        var recentPlaylistIds = LinkedList<String>()
        val recentPlaylistJson =
            PlaylistPreferenceUtils.defaultPrefs(this@PlaylistActivity).recentlyPlayedPlaylist
        if (!recentPlaylistJson.isNullOrEmpty()) {
            recentPlaylistIds = GsonBuilder().serializeNulls().create().fromJson(
                recentPlaylistJson,
                TypeToken.getParameterized(LinkedList::class.java, String::class.java).type
            )
            if (recentPlaylistIds.contains(mPlaylistId)) {
                recentPlaylistIds.remove(mPlaylistId)
            }
        }
        recentPlaylistIds.addFirst(mPlaylistId)
        PlaylistPreferenceUtils.defaultPrefs(this@PlaylistActivity).recentlyPlayedPlaylist =
            GsonBuilder().serializeNulls().create().toJson(recentPlaylistIds)
        val subItemMediaList = mutableListOf<MediaItem>()
        mPlaylist.items.forEach {
            if (PlaylistUtils.isPlaylistItemCached(it)) {
                val mediaItem = MediaItemUtil.buildMediaItem(
                    it,
                    mPlaylistId,
                    if (mPlaylistId == ConstantUtils.DEFAULT_PLAYLIST) resources.getString(R.string.playlist_play_later) else mPlaylist.name,
                )
                subItemMediaList.add(mediaItem)
            }
        }
        val selectedPlaylistItem = mPlaylist.items[position]
        browser.clearMediaItems()
        browser.addMediaItems(subItemMediaList)
        browser.seekTo(position, selectedPlaylistItem.lastPlayedPosition.toLong())
        browser.shuffleModeEnabled = isShuffle
        browser.prepare()
        browser.play()
        // mScope.launch {
            // val selectedPlaylistItem = mPlaylistModel.items[position]
            // val lastPlayedPositionModel =
            //     mPlaylistRepository.getLastPlayedPositionByPlaylistItemId(selectedPlaylistItem.id)

            // activity?.runOnUiThread {
                // browser.clearMediaItems()
                // browser.addMediaItems(subItemMediaList)
                // // browser.seekTo(position, lastPlayedPositionModel?.lastPlayedPosition ?: 0)
                // browser.seekTo(position, selectedPlaylistItem.lastPlayedPosition.toLong())
                // browser.shuffleModeEnabled = isShuffle
                // browser.prepare()
                // browser.play()
        //     }
        // }
        // val playlistPlayerFragment = PlaylistPlayerFragment.newInstance(mPlaylistModel)
        // parentFragmentManager.beginTransaction()
        //     .replace(android.R.id.content, playlistPlayerFragment)
        //     .addToBackStack(PlaylistFragment::class.simpleName).commit()
        val playlistPlayerActivityIntent = Intent(this@PlaylistActivity, PlaylistPlayerActivity::class.java)
        playlistPlayerActivityIntent.putExtra(ConstantUtils.PLAYLIST_ID, mPlaylistId);
        startActivity(playlistPlayerActivityIntent)
    }

    private fun deletePlaylistItems(playlistItems : List<PlaylistItem>) {
        playlistItems.forEach {
            // deleteHLSContent(playlistItem.getId());
            mPlaylistService?.removeItemFromPlaylist(mPlaylistId, it.id)
        }
        fetchPlaylistData()
    }

    // PlaylistItemClickListener callbacks
    override fun onPlaylistItemClick(position: Int) {
        openPlaylistPlayer(false, position)
    }

    override fun onPlaylistItemClickInEditMode(count: Int) {
        mPlaylistToolbar.updateSelectedItems(count)
    }

    override fun onPlaylistItemMenuClick(view: View, playlistItem: PlaylistItem) {
        MenuUtils.showPlaylistItemMenu(
            this@PlaylistActivity,
            supportFragmentManager,
            playlistItem = playlistItem,
            playlistId = mPlaylistId,
            playlistItemOptionsListener = this@PlaylistActivity
        )
    }

    // StartDragListener callbacks
    override fun onStartDrag(viewHolder: RecyclerView.ViewHolder) {
        mItemTouchHelper.startDrag(viewHolder)
    }

    // ItemInteractionListener callbacks
    override fun onItemDelete(position: Int) {
        val selectedPlaylistItem = mPlaylistItemAdapter?.currentList?.get(position)
        selectedPlaylistItem?.let {
            stopVideoPlayerOnDelete(it)
            deletePlaylistItems(arrayListOf(it))
            // mPlaylistViewModel.setDeletePlaylistItems(
            //     PlaylistModel(
            //         mPlaylistModel.id, mPlaylistModel.name, arrayListOf(
            //             it
            //         )
            //     )
            // )
        }
    }

    override fun onRemoveFromOffline(position: Int) {}

    override fun onShare(position: Int) {
        //Share model
        PlaylistUtils.showSharingDialog(this@PlaylistActivity, mPlaylist.items[position].pageSource.url)
    }

    // PlaylistOptionsListener callback
    override fun onPlaylistOptionClicked(playlistOptionsModel: PlaylistOptionsModel) {
        when (playlistOptionsModel.optionType) {
            PlaylistOptionsEnum.EDIT_PLAYLIST -> {
                mPlaylistItemAdapter?.setEditMode(true)
                mPlaylistToolbar.enableEditMode(true)
                mIvPlaylistOptions.visibility = View.GONE
            }

            PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS, PlaylistOptionsEnum.COPY_PLAYLIST_ITEMS -> {
                mPlaylistItemAdapter?.getSelectedItems()?.let {
                    PlaylistUtils.moveOrCopyModel = MoveOrCopyModel(
                        playlistOptionsModel.optionType, mPlaylistId, "", it
                    )
                }
                mPlaylistItemAdapter?.setEditMode(false)
                mPlaylistToolbar.enableEditMode(false)
                mIvPlaylistOptions.visibility = View.VISIBLE
                MoveOrCopyToPlaylistBottomSheet().show(supportFragmentManager, null)
            }

            PlaylistOptionsEnum.RENAME_PLAYLIST -> {
                val newActivityIntent = Intent(this@PlaylistActivity, NewPlaylistActivity::class.java);
                startActivity(newActivityIntent);
                // val newPlaylistFragment = NewPlaylistFragment.newInstance(
                //     PlaylistOptionsEnum.RENAME_PLAYLIST, mPlaylistModel
                // )
                // parentFragmentManager.beginTransaction()
                //     .replace(android.R.id.content, newPlaylistFragment)
                //     .addToBackStack(PlaylistFragment::class.simpleName).commit()
            }

            PlaylistOptionsEnum.DELETE_PLAYLIST -> {
                mMediaBrowser?.stop()
                mPlaylistService?.removePlaylist(mPlaylistId);
                finish()
            }

            else -> {
                //Do nothing
            }
        }
    }

    // // PlaylistItemOptionsListener callback
    // override fun onPlaylistItemOptionClicked(playlistItemOptionModel: PlaylistItemOptionModel) {
    //     when (playlistItemOptionModel.optionType) {
    //         PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM -> {
    //             playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
    //                 PlaylistUtils.showSharingDialog(
    //                     this@PlaylistActivity, it
    //                 )
    //             }
    //         }

    //         PlaylistOptionsEnum.OPEN_IN_NEW_TAB -> {
    //             playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
    //                 openPlaylistInTab(false, it)
    //             }
    //         }

    //         PlaylistOptionsEnum.OPEN_IN_PRIVATE_TAB -> {
    //             playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
    //                 openPlaylistInTab(true, it)
    //             }
    //         }

    //         PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM -> {
    //             playlistItemOptionModel.playlistItem?.let { 
    //                 stopVideoPlayerOnDelete(it)
    //                 deletePlaylistItems(arrayListOf(it))
    //             }
    //         }

    //         PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM, PlaylistOptionsEnum.COPY_PLAYLIST_ITEM -> {
    //             val moveOrCopyItems = ArrayList<PlaylistItem>()
    //             playlistItemOptionModel.playlistItem?.let { moveOrCopyItems.add(it) }
    //             PlaylistUtils.moveOrCopyModel =
    //                     MoveOrCopyModel(playlistItemOptionModel.optionType,mPlaylistId , "", moveOrCopyItems)
    //             MoveOrCopyToPlaylistBottomSheet().show(supportFragmentManager, null)
    //         }
    //         else -> {
    //             //Do nothing
    //         }
    //     }
    // }

    override fun onMediaFileDownloadProgressed(id:String, totalBytes:Long, receivedBytes:Long, percentComplete:Byte, timeRemaining:String) {
        mPlaylistItemAdapter?.updatePlaylistItemDownloadProgress(HlsContentProgressModel(id, totalBytes, receivedBytes, "" + percentComplete))
    }

    override fun onItemCached(playlistItem:PlaylistItem) {
        Log.e(TAG, "onItemCached")
        mPlaylistItemAdapter?.updatePlaylistItem(playlistItem)
    }

    override fun onItemUpdated(playlistItem:PlaylistItem) {
        Log.e(TAG, "onItemUpdated")
        mPlaylistItemAdapter?.updatePlaylistItem(playlistItem)
    }

    override fun onPlaylistUpdated(playlist:Playlist) {
        Log.e(TAG, "onPlaylistUpdated")
        fetchPlaylistData()
    }

    override fun onEvent(eventType: Int , id : String) {
        if (eventType == PlaylistEvent.ITEM_MOVED && id.equals(mPlaylistId)) {
            fetchPlaylistData()
        }
    }

    override fun deletePlaylistItem(playlistItemOptionModel: PlaylistItemOptionModel) {
        playlistItemOptionModel.playlistItem?.let { 
            stopVideoPlayerOnDelete(it)
            deletePlaylistItems(arrayListOf(it))
        }
    }
}

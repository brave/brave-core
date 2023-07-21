/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.fragment

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.text.TextUtils
import android.text.format.Formatter
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import androidx.appcompat.widget.LinearLayoutCompat
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.RecyclerView
import org.chromium.chrome.browser.playlist.kotlin.PlaylistDownloadUtils
import org.chromium.chrome.browser.playlist.kotlin.PlaylistVideoService
import com.brave.playlist.PlaylistViewModel
import org.chromium.chrome.R
import com.brave.playlist.adapter.recyclerview.PlaylistItemAdapter
import com.brave.playlist.enums.PlaylistOptionsEnum
import com.brave.playlist.extension.afterMeasured
import com.brave.playlist.listener.ItemInteractionListener
import com.brave.playlist.listener.PlaylistItemClickListener
import com.brave.playlist.listener.PlaylistItemOptionsListener
import com.brave.playlist.listener.PlaylistOptionsListener
import com.brave.playlist.listener.StartDragListener
import com.brave.playlist.fragment.NewPlaylistFragment
import com.brave.playlist.model.MoveOrCopyModel
import com.brave.playlist.model.PlaylistItemModel
import com.brave.playlist.model.PlaylistItemOptionModel
import com.brave.playlist.model.PlaylistModel
import com.brave.playlist.model.PlaylistOptionsModel
import com.brave.playlist.util.ConnectionUtils
import com.brave.playlist.util.ConstantUtils.CURRENT_PLAYING_ITEM_ID
import com.brave.playlist.util.ConstantUtils.DEFAULT_PLAYLIST
import com.brave.playlist.util.ConstantUtils.TAG
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker
import com.brave.playlist.util.MediaUtils
import com.brave.playlist.util.MenuUtils
import com.brave.playlist.util.PlaylistItemGestureHelper
import com.brave.playlist.util.PlaylistPreferenceUtils
import com.brave.playlist.util.PlaylistPreferenceUtils.getLatestPlaylistItem
import com.brave.playlist.util.PlaylistPreferenceUtils.recentlyPlayedPlaylist
import com.brave.playlist.util.PlaylistPreferenceUtils.rememberListPlaybackPosition
import com.brave.playlist.util.PlaylistUtils
import com.brave.playlist.view.PlaylistToolbar
import androidx.lifecycle.ViewModelStoreOwner
import com.bumptech.glide.Glide
import com.google.gson.GsonBuilder
import com.google.gson.reflect.TypeToken
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import java.io.IOException
import java.util.LinkedList

class PlaylistFragment : Fragment(R.layout.fragment_playlist), ItemInteractionListener,
    StartDragListener, PlaylistOptionsListener, PlaylistItemOptionsListener,
    PlaylistItemClickListener {
    private val mScope = CoroutineScope(Job() + Dispatchers.IO)

    private lateinit var mPlaylistModel: PlaylistModel
    private lateinit var mPlaylistViewModel: PlaylistViewModel
    private var mPlaylistItemAdapter: PlaylistItemAdapter? = null
    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private lateinit var mRvPlaylist: RecyclerView
    private lateinit var mTvTotalMediaCount: TextView
    private lateinit var mTvPlaylistName: TextView
    private lateinit var mLayoutPlayMedia: LinearLayoutCompat
    private lateinit var mIvPlaylistOptions: ImageView
    private lateinit var mProgressBar: ProgressBar
    private lateinit var mLayoutShuffleMedia: LinearLayoutCompat
    private lateinit var mIvPlaylistCover: AppCompatImageView
    private lateinit var mTvPlaylistTotalSize: AppCompatTextView
    private lateinit var mItemTouchHelper: ItemTouchHelper

    private lateinit var mEmptyView: View
    private lateinit var mPlaylistView: View

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        mPlaylistViewModel = ViewModelProvider(requireActivity() as ViewModelStoreOwner)[PlaylistViewModel::class.java]

        mEmptyView = view.findViewById(R.id.empty_view)
        mPlaylistView = view.findViewById(R.id.playlist_view)

        mPlaylistToolbar = view.findViewById(R.id.playlistToolbar)
        mPlaylistToolbar.setOptionsButtonClickListener {

        }
        mPlaylistToolbar.setExitEditModeClickListener {
            mPlaylistItemAdapter?.setEditMode(false)
            mPlaylistToolbar.enableEditMode(false)
            mIvPlaylistOptions.visibility = View.VISIBLE
            //Reorder list
            mPlaylistItemAdapter?.getPlaylistItems()
                ?.let { playlistItems -> mPlaylistViewModel.reorderPlaylistItems(playlistItems) }
        }
        mPlaylistToolbar.setMoveClickListener { actionView ->
            mPlaylistItemAdapter?.getSelectedItems()?.let {
                if (it.size > 0) {
                    MenuUtils.showMoveOrCopyMenu(
                        actionView,
                        parentFragmentManager,
                        it,
                        this
                    )
                } else {
                    Toast.makeText(
                        activity,
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
                    mPlaylistViewModel.setDeletePlaylistItems(
                        PlaylistModel(
                            mPlaylistModel.id,
                            mPlaylistModel.name,
                            it
                        )
                    )
                    mPlaylistItemAdapter?.setEditMode(false)
                    mPlaylistToolbar.enableEditMode(false)
                    mIvPlaylistOptions.visibility = View.VISIBLE
                } else {
                    Toast.makeText(
                        activity,
                        getString(R.string.playlist_please_select_items),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }
        mRvPlaylist = view.findViewById(R.id.rvPlaylists)
        mTvTotalMediaCount = view.findViewById(R.id.tvTotalMediaCount)
        mTvPlaylistName = view.findViewById(R.id.tvPlaylistName)
        mLayoutPlayMedia = view.findViewById(R.id.layoutPlayMedia)
        mLayoutPlayMedia.isVisible
        mLayoutShuffleMedia = view.findViewById(R.id.layoutShuffleMedia)
        mIvPlaylistOptions = view.findViewById(R.id.ivPlaylistOptions)
        mIvPlaylistCover = view.findViewById(R.id.ivPlaylistCover)
        mTvPlaylistTotalSize = view.findViewById(R.id.tvPlaylistTotalSize)
        mProgressBar = view.findViewById(R.id.progressBar)
        mProgressBar.visibility = View.VISIBLE
        mRvPlaylist.visibility = View.GONE

        PlaylistVideoService.currentPlayingItem.observe(viewLifecycleOwner) { currentPlayingItemId ->
            if (!currentPlayingItemId.isNullOrEmpty()) {
                mPlaylistItemAdapter?.updatePlayingStatus(currentPlayingItemId)
            }
        }

        mPlaylistViewModel.playlistData.observe(viewLifecycleOwner) { playlistData ->
            Log.e(TAG, playlistData.toString())
            var totalFileSize = 0L
            mPlaylistModel = playlistData

            view.findViewById<Button>(R.id.btBrowseForMedia).setOnClickListener {
                activity?.finish()
            }

            playlistData.items.forEach { playlistItemModel ->
                PlaylistDownloadUtils.startDownloadRequest(requireContext(), playlistItemModel)
            }

            if (mPlaylistModel.items.isNotEmpty()) {
                Glide.with(requireContext())
                    .load(mPlaylistModel.items[0].thumbnailPath)
                    .placeholder(R.drawable.ic_playlist_placeholder)
                    .error(R.drawable.ic_playlist_placeholder)
                    .into(mIvPlaylistCover)
                mLayoutPlayMedia.setOnClickListener {
                    if (PlaylistPreferenceUtils.defaultPrefs(requireContext()).rememberListPlaybackPosition && !TextUtils.isEmpty(
                            PlaylistPreferenceUtils.defaultPrefs(requireContext())
                                .getLatestPlaylistItem(mPlaylistModel.id)
                        )
                    ) {
                        mPlaylistModel.items.forEach { playlistToOpen ->
                            if (playlistToOpen.id == PlaylistPreferenceUtils.defaultPrefs(
                                    requireContext()
                                ).getLatestPlaylistItem(mPlaylistModel.id)
                            ) {
                                openPlaylistPlayer(playlistToOpen)
                                return@forEach
                            }
                        }
                    } else {
                        openPlaylistPlayer(mPlaylistModel.items[0])
                    }
                }

                mLayoutShuffleMedia.setOnClickListener {
                    openPlaylistPlayer(
                        mPlaylistModel.items[(0 until mPlaylistModel.items.size).shuffled().last()]
                    )
                }

                mTvTotalMediaCount.text =
                    getString(
                        R.string.playlist_number_of_items,
                        mPlaylistModel.items.size.toString()
                    )

                mTvPlaylistName.text =
                    if (mPlaylistModel.id == DEFAULT_PLAYLIST) resources.getString(R.string.playlist_play_later) else mPlaylistModel.name

                mScope.launch {
                    mPlaylistModel.items.forEach {
                        try {
                            if (it.isCached) {
                                val fileSize = MediaUtils.getFileSizeFromUri(
                                    view.context,
                                    Uri.parse(it.mediaPath)
                                )
                                it.fileSize = fileSize
                                totalFileSize += fileSize
                            }
                        } catch (ex: IOException) {
                            Log.e(TAG, ex.message.toString())
                        }
                    }

                    activity?.runOnUiThread {
                        if (totalFileSize > 0) {
                            mTvPlaylistTotalSize.text =
                                Formatter.formatShortFileSize(view.context, totalFileSize)
                        }
                        mPlaylistItemAdapter = PlaylistItemAdapter(
                            mPlaylistModel.items.toMutableList(),
                            this@PlaylistFragment,
                            this@PlaylistFragment
                        )
                        mPlaylistItemAdapter?.let {
                            val callback =
                                PlaylistItemGestureHelper(
                                    view.context,
                                    mRvPlaylist,
                                    it,
                                    this@PlaylistFragment
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

                        mPlaylistViewModel.downloadProgress.observe(viewLifecycleOwner) {
                            mPlaylistItemAdapter?.updatePlaylistItemDownloadProgress(it)
                        }

                        mPlaylistViewModel.playlistItemEventUpdate.observe(viewLifecycleOwner) {
                            mPlaylistItemAdapter?.updatePlaylistItem(it)
                        }

                        mRvPlaylist.afterMeasured {
                            PlaylistVideoService.CURRENTLY_PLAYED_ITEM_ID?.let {
                                mPlaylistItemAdapter?.updatePlayingStatus(
                                    it
                                )
                                if (!arguments?.getString(CURRENT_PLAYING_ITEM_ID)
                                        .isNullOrEmpty() && mPlaylistModel.items.isNotEmpty()
                                ) {
                                    mPlaylistModel.items.forEach { item ->
                                        if (item.id == arguments?.getString(CURRENT_PLAYING_ITEM_ID)) {
                                            Log.e(TAG, item.id + " : " + item.name)
                                            openPlaylistPlayer(item)
                                            arguments?.putString(CURRENT_PLAYING_ITEM_ID, "")
                                            return@forEach
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                mIvPlaylistCover.setImageResource(R.drawable.ic_playlist_placeholder)
                mEmptyView.visibility = View.VISIBLE
                mPlaylistView.visibility = View.GONE
            }
        }

        mIvPlaylistOptions.setOnClickListener {
            MenuUtils.showPlaylistMenu(
                view.context, parentFragmentManager,
                mPlaylistModel, this, mPlaylistModel.id == DEFAULT_PLAYLIST
            )
        }
    }

    override fun onItemDelete(position: Int) {
        val selectedPlaylistItem = mPlaylistItemAdapter?.getPlaylistItems()?.get(position)
        selectedPlaylistItem?.let {
            stopVideoPlayerOnDelete(it)
            mPlaylistViewModel.setDeletePlaylistItems(
                PlaylistModel(
                    mPlaylistModel.id,
                    mPlaylistModel.name,
                    arrayListOf(
                        it
                    )
                )
            )
        }
    }

    override fun onRemoveFromOffline(position: Int) {
        val selectedPlaylistItem = mPlaylistItemAdapter?.getPlaylistItems()?.get(position)
        selectedPlaylistItem?.let {
            stopVideoPlayerOnDelete(it)
            val playlistOptionsEnumModel = PlaylistItemOptionModel(
                requireContext().resources.getString(R.string.playlist_delete_item_offline_data),
                R.drawable.ic_remove_offline_data_playlist,
                PlaylistOptionsEnum.DELETE_ITEMS_OFFLINE_DATA,
                playlistItemModel = it,
                playlistId = mPlaylistModel.id
            )
            mPlaylistViewModel.setPlaylistItemOption(playlistOptionsEnumModel)
        }
    }

    override fun onShare(position: Int) {
        super.onShare(position)
        val playlistItemModel = mPlaylistModel.items[position]
        //Share model
        PlaylistUtils.showSharingDialog(requireContext(), playlistItemModel.pageSource)
    }

    override fun onStartDrag(viewHolder: RecyclerView.ViewHolder) {
        mItemTouchHelper.startDrag(viewHolder)
    }

    override fun onPlaylistItemClick(playlistItemModel: PlaylistItemModel) {
        openPlaylistPlayer(playlistItemModel)
    }

    override fun onPlaylistItemClick(count: Int) {
        mPlaylistToolbar.updateSelectedItems(count)
    }

    override fun onPlaylistItemMenuClick(view: View, playlistItemModel: PlaylistItemModel) {
        MenuUtils.showPlaylistItemMenu(
            view.context,
            parentFragmentManager,
            playlistItemModel = playlistItemModel,
            playlistId = playlistItemModel.playlistId,
            playlistItemOptionsListener = this,
            mPlaylistModel.name == DEFAULT_PLAYLIST
        )
    }

    private fun openPlaylistPlayer(selectedPlaylistItemModel: PlaylistItemModel) {
        if (!selectedPlaylistItemModel.isCached && !ConnectionUtils.isDeviceOnline(requireContext())) {
            Toast.makeText(
                requireContext(),
                getString(R.string.playlist_offline_message),
                Toast.LENGTH_SHORT
            ).show()
            return
        }

        if (!selectedPlaylistItemModel.isCached && PlaylistUtils.isMediaSourceExpired(
                selectedPlaylistItemModel.mediaSrc
            )
        ) {
            Toast.makeText(
                requireContext(),
                getString(R.string.playlist_item_expired_message),
                Toast.LENGTH_SHORT
            ).show()
            val playlistItemOptionModel = PlaylistItemOptionModel(
                requireContext().resources.getString(R.string.playlist_open_in_private_tab),
                R.drawable.ic_private_tab,
                PlaylistOptionsEnum.RECOVER_PLAYLIST_ITEM,
                playlistItemModel = selectedPlaylistItemModel,
                playlistId = selectedPlaylistItemModel.playlistId
            )
            mPlaylistViewModel.setPlaylistItemOption(playlistItemOptionModel)
        } else {
            var recentPlaylistIds = LinkedList<String>()
            val recentPlaylistJson =
                PlaylistPreferenceUtils.defaultPrefs(requireContext()).recentlyPlayedPlaylist
            if (!recentPlaylistJson.isNullOrEmpty()) {
                recentPlaylistIds = GsonBuilder().serializeNulls().create().fromJson(
                    recentPlaylistJson,
                    TypeToken.getParameterized(LinkedList::class.java, String::class.java).type
                )
                if (recentPlaylistIds.contains(mPlaylistModel.id)) {
                    recentPlaylistIds.remove(mPlaylistModel.id)
                }
            }
            recentPlaylistIds.addFirst(mPlaylistModel.id)
            PlaylistPreferenceUtils.defaultPrefs(requireContext()).recentlyPlayedPlaylist =
                GsonBuilder().serializeNulls().create().toJson(recentPlaylistIds)

            BraveVpnNativeWorker.getInstance().queryPrompt(
                selectedPlaylistItemModel.mediaSrc,
                "GET");

            activity?.stopService(Intent(requireContext(), PlaylistVideoService::class.java))
            val playlistPlayerFragment =
                PlaylistPlayerFragment.newInstance(
                    selectedPlaylistItemModel.id,
                    mPlaylistModel
                )
            parentFragmentManager.beginTransaction()
                .replace(android.R.id.content, playlistPlayerFragment)
                .addToBackStack(PlaylistFragment::class.simpleName)
                .commit()
        }
    }

    override fun onOptionClicked(playlistOptionsModel: PlaylistOptionsModel) {
        when (playlistOptionsModel.optionType) {
            PlaylistOptionsEnum.EDIT_PLAYLIST -> {
                mPlaylistItemAdapter?.setEditMode(true)
                mPlaylistToolbar.enableEditMode(true)
                mIvPlaylistOptions.visibility = View.GONE
            }

            PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS, PlaylistOptionsEnum.COPY_PLAYLIST_ITEMS -> {
                mPlaylistItemAdapter?.getSelectedItems()?.let {
                    PlaylistUtils.moveOrCopyModel = MoveOrCopyModel(
                        playlistOptionsModel.optionType,
                        "",
                        it
                    )
                }
                mPlaylistItemAdapter?.setEditMode(false)
                mPlaylistToolbar.enableEditMode(false)
                mIvPlaylistOptions.visibility = View.VISIBLE
            }

            PlaylistOptionsEnum.RENAME_PLAYLIST -> {
                val newPlaylistFragment = NewPlaylistFragment.newInstance(
                    PlaylistOptionsEnum.RENAME_PLAYLIST,
                    mPlaylistModel
                )
                parentFragmentManager
                    .beginTransaction()
                    .replace(android.R.id.content, newPlaylistFragment)
                    .addToBackStack(PlaylistFragment::class.simpleName)
                    .commit()
            }

            PlaylistOptionsEnum.DELETE_PLAYLIST -> {
                activity?.stopService(Intent(requireContext(), PlaylistVideoService::class.java))
            }

            else -> {
                //Do nothing
            }
        }
        mPlaylistViewModel.setPlaylistOption(playlistOptionsModel)
    }

    override fun onOptionClicked(playlistItemOptionModel: PlaylistItemOptionModel) {
        if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM) {
            playlistItemOptionModel.playlistItemModel?.pageSource?.let {
                PlaylistUtils.showSharingDialog(
                    requireContext(),
                    it
                )
            }
        } else {
            if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM || playlistItemOptionModel.optionType == PlaylistOptionsEnum.COPY_PLAYLIST_ITEM) {
                val moveOrCopyItems = ArrayList<PlaylistItemModel>()
                playlistItemOptionModel.playlistItemModel?.let { moveOrCopyItems.add(it) }
                PlaylistUtils.moveOrCopyModel =
                    MoveOrCopyModel(playlistItemOptionModel.optionType, "", moveOrCopyItems)
            } else if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.DELETE_ITEMS_OFFLINE_DATA || playlistItemOptionModel.optionType == PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM) {
                playlistItemOptionModel.playlistItemModel?.let { stopVideoPlayerOnDelete(it) }
            }
            mPlaylistViewModel.setPlaylistItemOption(playlistItemOptionModel)
        }
    }

    private fun stopVideoPlayerOnDelete(selectedPlaylistItem: PlaylistItemModel) {
        if (PlaylistVideoService.CURRENTLY_PLAYED_ITEM_ID == selectedPlaylistItem.id) {
            activity?.stopService(Intent(requireContext(), PlaylistVideoService::class.java))
        }
    }
}

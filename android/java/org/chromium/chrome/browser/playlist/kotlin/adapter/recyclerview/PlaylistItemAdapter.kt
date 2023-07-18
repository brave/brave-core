/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview

import android.annotation.SuppressLint
import android.graphics.Color
import android.text.format.Formatter
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistItemEventEnum
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemClickListener
import org.chromium.chrome.browser.playlist.kotlin.listener.StartDragListener
import org.chromium.chrome.browser.playlist.kotlin.model.DownloadProgressModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemEventModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel
import com.bumptech.glide.Glide

class PlaylistItemAdapter(
    mediaItemList: MutableList<PlaylistItemModel>,
    private val playlistItemClickListener: PlaylistItemClickListener?,
    private val startDragListener: StartDragListener? = null,
) :
    AbstractRecyclerViewAdapter<PlaylistItemAdapter.MediaItemViewHolder, PlaylistItemModel>(
        mediaItemList
    ) {
    private var editMode = false
    private var isBottomLayout = false

    init {
        editMode = false
    }

    private var allViewHolderViews = HashMap<String, View>()
    fun updatePlaylistItemDownloadProgress(downloadProgressModel: DownloadProgressModel) {
        val view = allViewHolderViews[downloadProgressModel.playlistItemId]
        val ivMediaStatus: AppCompatImageView? = view?.findViewById(R.id.ivMediaStatus)
        val tvMediaDownloadProgress: AppCompatTextView? =
            view?.findViewById(R.id.tvMediaDownloadProgress)
        if (downloadProgressModel.totalBytes != downloadProgressModel.receivedBytes) {
            ivMediaStatus?.visibility = View.GONE
            tvMediaDownloadProgress?.visibility = View.VISIBLE
            tvMediaDownloadProgress?.text =
                view?.resources?.getString(R.string.playlist_percentage_text)
                    ?.let { String.format(it, downloadProgressModel.percentComplete.toString()) }
        }
    }

    fun updatePlaylistItem(playlistItemEventModel: PlaylistItemEventModel) {
        val view = allViewHolderViews[playlistItemEventModel.playlistItemModel.id]
        val ivMediaStatus: AppCompatImageView? = view?.findViewById(R.id.ivMediaStatus)
        val tvMediaDownloadProgress: AppCompatTextView? =
            view?.findViewById(R.id.tvMediaDownloadProgress)
        if (playlistItemEventModel.playlistItemEventEnum == PlaylistItemEventEnum.ITEM_CACHED) {
            tvMediaDownloadProgress?.visibility = View.GONE
            ivMediaStatus?.visibility = View.VISIBLE
            ivMediaStatus?.setImageResource(R.drawable.ic_downloaded)
        } else if (playlistItemEventModel.playlistItemEventEnum == PlaylistItemEventEnum.ITEM_LOCAL_DATA_REMOVED) {
            tvMediaDownloadProgress?.visibility = View.GONE
            ivMediaStatus?.visibility = View.VISIBLE
            ivMediaStatus?.setImageResource(R.drawable.ic_offline)
        }
        var index: Int? = null
        for (i in 0 until itemList.size) {
            if (itemList[i].id == playlistItemEventModel.playlistItemModel.id) {
                index = i
                break
            }
        }

        index?.let {
            playlistItemEventModel.playlistItemModel.fileSize = itemList[it].fileSize
            itemList[it] = playlistItemEventModel.playlistItemModel
            notifyItemChanged(it)
        }
    }

    fun updatePlayingStatus(playlistItemId: String) {
        if (getEditMode()) {
            return
        }
        val currentPlayingItemView = allViewHolderViews[playlistItemId]
        val ivMediaPlayingStatusCurrent: AppCompatImageView? =
            currentPlayingItemView?.findViewById(R.id.ivMediaPlayingStatus)
        ivMediaPlayingStatusCurrent?.visibility = View.VISIBLE
        val mediaTitleCurrent: AppCompatTextView? =
            currentPlayingItemView?.findViewById(R.id.tvMediaTitle)
        mediaTitleCurrent?.setTextColor(currentPlayingItemView.context.getColor(R.color.brave_theme_color))
        allViewHolderViews.keys.forEach {
            if (it != playlistItemId) {
                val view = allViewHolderViews[it]
                val ivMediaPlayingStatus: AppCompatImageView? =
                    view?.findViewById(R.id.ivMediaPlayingStatus)
                ivMediaPlayingStatus?.visibility = View.GONE
                val mediaTitle: AppCompatTextView? = view?.findViewById(R.id.tvMediaTitle)
                mediaTitle?.setTextColor(view.context.getColor(R.color.playlist_text_color))
            }
        }
    }

    fun setEditMode(enable: Boolean) {
        editMode = enable
        itemList.forEach { it.isSelected = false }
        notifyItemRangeChanged(0, size)
    }

    fun getEditMode(): Boolean {
        return editMode
    }

    fun setBottomLayout() {
        isBottomLayout = true
    }

    inner class MediaItemViewHolder(view: View) :
        AbstractViewHolder<PlaylistItemModel>(view) {
        private val ivMediaThumbnail: AppCompatImageView
        private val tvMediaTitle: AppCompatTextView
        private val tvMediaDuration: AppCompatTextView
        private val tvMediaFileSize: AppCompatTextView
        private val ivDragMedia: AppCompatImageView
        private val ivMediaOptions: AppCompatImageView
        private val ivMediaSelected: AppCompatImageView
        private val ivMediaStatus: AppCompatImageView
        private val tvMediaDownloadProgress: AppCompatTextView

        init {
            ivMediaThumbnail = view.findViewById(R.id.ivMediaThumbnail)
            tvMediaTitle = view.findViewById(R.id.tvMediaTitle)
            tvMediaDuration = view.findViewById(R.id.tvMediaDuration)
            tvMediaFileSize = view.findViewById(R.id.tvMediaFileSize)
            ivDragMedia = view.findViewById(R.id.ivDragMedia)
            ivMediaOptions = view.findViewById(R.id.ivMediaOptions)
            ivMediaSelected = view.findViewById(R.id.ivMediaSelected)
            ivMediaStatus = view.findViewById(R.id.ivMediaStatus)
            tvMediaDownloadProgress = view.findViewById(R.id.tvMediaDownloadProgress)
        }

        @SuppressLint("ClickableViewAccessibility")
        override fun onBind(position: Int, model: PlaylistItemModel) {
            setViewOnSelected(model.isSelected)
            ivMediaStatus.setImageResource(if (model.isCached) R.drawable.ic_downloaded else R.drawable.ic_offline)
            ivMediaStatus.visibility = if (!editMode) View.VISIBLE else View.GONE
            tvMediaTitle.text = model.name

            if (model.thumbnailPath.isNotEmpty()) {
                Glide.with(itemView.context)
                    .asBitmap()
                    .placeholder(R.drawable.ic_playlist_item_placeholder)
                    .error(R.drawable.ic_playlist_item_placeholder)
                    .load(model.thumbnailPath)
                    .into(ivMediaThumbnail)
            } else {
                ivMediaThumbnail.setImageResource(R.drawable.ic_playlist_item_placeholder)
            }

            if (model.isCached) {
                val fileSize = model.fileSize
                tvMediaFileSize.text =
                    Formatter.formatShortFileSize(itemView.context, fileSize)
            }

            tvMediaFileSize.visibility = if (model.isCached) View.VISIBLE else View.GONE

            if (model.duration.isNotEmpty()) {
                val duration = model.duration.toLongOrNull()
                if (duration != null) {
                    val milliseconds = (duration / 1000) % 1000
                    val seconds = ((duration / 1000) - milliseconds) / 1000 % 60
                    val minutes =
                        (((duration / 1000) - milliseconds) / 1000 - seconds) / 60 % 60
                    val hours =
                        ((((duration / 1000) - milliseconds) / 1000 - seconds) / 60 - minutes) / 60

                    val hourTime: String = if (hours > 0) itemView.context.resources.getString(
                        R.string.playlist_time_text,
                        hours.toString()
                    ) else ""
                    val minuteTime: String = if (minutes > 0) itemView.context.resources.getString(
                        R.string.playlist_time_text,
                        minutes.toString()
                    ) else ""
                    tvMediaDuration.visibility = View.VISIBLE
                    tvMediaDuration.text = itemView.context.resources.getString(
                        R.string.playlist_duration_text,
                        hourTime,
                        minuteTime,
                        seconds.toString()
                    )
                }
            }
            ivMediaOptions.visibility = if (!editMode) View.VISIBLE else View.GONE
            ivMediaOptions.setOnClickListener {
                playlistItemClickListener?.onPlaylistItemMenuClick(
                    view = it,
                    playlistItemModel = model
                )
            }
            ivDragMedia.visibility = if (editMode) View.VISIBLE else View.GONE
            itemView.setOnClickListener {
                if (editMode) {
                    model.isSelected = !model.isSelected
                    setViewOnSelected(model.isSelected)
                    var count = 0
                    itemList.forEach {
                        if (it.isSelected) {
                            count++
                        }
                    }
                    playlistItemClickListener?.onPlaylistItemClick(count)
                } else {
                    if (isBottomLayout) playlistItemClickListener?.onPlaylistItemClick(position) else playlistItemClickListener?.onPlaylistItemClick(
                        playlistItemModel = model
                    )
                }
            }
            ivDragMedia.setOnTouchListener { _, event ->
                if (event.actionMasked == MotionEvent.ACTION_DOWN)
                    startDragListener?.onStartDrag(this)
                false
            }

            allViewHolderViews[model.id] = itemView
        }

        override fun isSelected(position: Int): Boolean {
            return position in 0 until size && itemList[position].isSelected
        }

        private fun setViewOnSelected(isSelected: Boolean) {
            ivMediaSelected.visibility = if (isSelected) View.VISIBLE else View.GONE
            itemView.setBackgroundColor(if (isSelected) itemView.context.getColor(R.color.selected_media) else Color.TRANSPARENT)
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MediaItemViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.playlist_item_layout, parent, false)
        return MediaItemViewHolder(view)
    }

    fun getSelectedItems(): ArrayList<PlaylistItemModel> {
        val selectedItems = arrayListOf<PlaylistItemModel>()
        itemList.forEach {
            if (it.isSelected) {
                selectedItems.add(it)
            }
        }
        return selectedItems
    }

    fun getPlaylistItems(): MutableList<PlaylistItemModel> {
        return itemList
    }
}

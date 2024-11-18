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
import android.util.Log
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemClickListener
import org.chromium.chrome.browser.playlist.kotlin.listener.StartDragListener
import org.chromium.chrome.browser.playlist.kotlin.model.HlsContentProgressModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import com.bumptech.glide.Glide
import com.google.android.material.progressindicator.CircularProgressIndicator


class PlaylistItemAdapter(
    private val playlistItemClickListener: PlaylistItemClickListener?,
    private val startDragListener: StartDragListener? = null,
) : AbstractRecyclerViewAdapter<PlaylistItemModel, PlaylistItemAdapter.MediaItemViewHolder>() {
    private var editMode = false
    private var isBottomLayout = false
    private var allViewHolderViews = HashMap<String, View>()

    init {
        editMode = false
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MediaItemViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.playlist_item_layout, parent, false)
        return MediaItemViewHolder(view)
    }

    fun setEditMode(enable: Boolean) {
        editMode = enable
        currentList.forEach { it.isSelected = false }
        notifyItemRangeChanged(0, itemCount)
    }

    fun getEditMode(): Boolean {
        return editMode
    }

    fun setBottomLayout() {
        isBottomLayout = true
    }

    inner class MediaItemViewHolder(view: View) : AbstractViewHolder<PlaylistItemModel>(view) {
        private val ivMediaThumbnail: AppCompatImageView
        private val tvMediaTitle: AppCompatTextView
        private val tvMediaDuration: AppCompatTextView
        private val tvMediaFileSize: AppCompatTextView
        private val ivDragMedia: AppCompatImageView
        private val ivMediaOptions: AppCompatImageView
        private val ivMediaSelected: AppCompatImageView

        private val tvMediaDownloadProgress: AppCompatTextView

        init {
            ivMediaThumbnail = view.findViewById(R.id.ivMediaThumbnail)
            tvMediaTitle = view.findViewById(R.id.tvMediaTitle)
            tvMediaDuration = view.findViewById(R.id.tvMediaDuration)
            tvMediaFileSize = view.findViewById(R.id.tvMediaFileSize)
            ivDragMedia = view.findViewById(R.id.ivDragMedia)
            ivMediaOptions = view.findViewById(R.id.ivMediaOptions)
            ivMediaSelected = view.findViewById(R.id.ivMediaSelected)
            tvMediaDownloadProgress = view.findViewById(R.id.tvMediaDownloadProgress)
        }

        @SuppressLint("ClickableViewAccessibility")
        override fun onBind(position: Int, model: PlaylistItemModel) {
            setViewOnSelected(model.isSelected)
            if (PlaylistUtils.isPlaylistItemCached(model)) {
                setAlphaForViews(itemView as ViewGroup, 1.0f)
                tvMediaDownloadProgress.visibility = View.GONE
            } else {
                setAlphaForViews(itemView as ViewGroup, 0.4f)
            }
            tvMediaTitle.text = model.name

            if (model.thumbnailPath.isNotEmpty()) {
                Glide.with(itemView.context).asBitmap()
                    .placeholder(R.drawable.ic_playlist_item_placeholder)
                    .error(R.drawable.ic_playlist_item_placeholder).load(model.thumbnailPath)
                    .into(ivMediaThumbnail)
            } else {
                ivMediaThumbnail.setImageResource(R.drawable.ic_playlist_item_placeholder)
            }

            if (model.isCached) {
                val fileSize = model.mediaFileBytes
                tvMediaFileSize.text = Formatter.formatShortFileSize(itemView.context, fileSize)
            }

            tvMediaFileSize.visibility =
                if (PlaylistUtils.isPlaylistItemCached(model)) View.VISIBLE else View.GONE
            tvMediaDuration.visibility =
                if (PlaylistUtils.isPlaylistItemCached(model)) View.VISIBLE else View.GONE

            if (model.duration.isNotEmpty()) {
                val duration = model.duration.toLongOrNull()
                if (duration != null && duration > 0) {
                    val milliseconds = (duration / 1000) % 1000
                    val seconds = ((duration / 1000) - milliseconds) / 1000 % 60
                    val minutes = (((duration / 1000) - milliseconds) / 1000 - seconds) / 60 % 60
                    val hours =
                        ((((duration / 1000) - milliseconds) / 1000 - seconds) / 60 - minutes) / 60
                    var durationText = ""
                    if (hours > 0) {
                        durationText = durationText.plus(String.format("%02d:", hours))
                    }
                    durationText = durationText.plus(String.format("%02d:", minutes))
                    durationText = durationText.plus(String.format("%02d", seconds))
                    tvMediaDuration.visibility = View.VISIBLE
                    tvMediaDuration.text = durationText
                }
            }
            ivMediaOptions.visibility = if (!editMode) View.VISIBLE else View.GONE
            ivMediaOptions.setOnClickListener {
                if (!PlaylistUtils.isPlaylistItemCached(model)) {
                    return@setOnClickListener
                }
                playlistItemClickListener?.onPlaylistItemMenuClick(
                    view = it, playlistItemModel = model
                )
            }
            ivDragMedia.visibility = if (editMode) View.VISIBLE else View.GONE
            itemView.setOnClickListener {
                if (!PlaylistUtils.isPlaylistItemCached(model)) {
                    return@setOnClickListener
                }
                if (editMode) {
                    model.isSelected = !model.isSelected
                    setViewOnSelected(model.isSelected)
                    var count = 0
                    currentList.forEach {
                        if (it.isSelected) {
                            count++
                        }
                    }
                    playlistItemClickListener?.onPlaylistItemClickInEditMode(count)
                } else {
                    playlistItemClickListener?.onPlaylistItemClick(position)
                }
            }
            ivDragMedia.setOnTouchListener { _, event ->
                if (event.actionMasked == MotionEvent.ACTION_DOWN) startDragListener?.onStartDrag(
                    this
                )
                false
            }
            allViewHolderViews[model.id] = itemView
        }

        override fun isSelected(position: Int): Boolean {
            return position in 0 until itemCount && currentList[position].isSelected
        }

        private fun setViewOnSelected(isSelected: Boolean) {
            ivMediaSelected.visibility = if (isSelected) View.VISIBLE else View.GONE
            itemView.setBackgroundColor(if (isSelected) itemView.context.getColor(R.color.selected_media) else Color.TRANSPARENT)
        }
    }

    private fun setAlphaForViews(parentLayout: ViewGroup, alphaValue: Float) {
        for (i in 0 until parentLayout.childCount) {
            val child: View = parentLayout.getChildAt(i)
            if (child.id != R.id.processing_progress_bar) {
                child.alpha = alphaValue
            }
        }
    }

    fun getSelectedItems(): ArrayList<PlaylistItemModel> {
        val selectedItems = arrayListOf<PlaylistItemModel>()
        currentList.forEach {
            if (it.isSelected) {
                selectedItems.add(it)
            }
        }
        return selectedItems
    }

    fun updatePlaylistItemDownloadProgress(hlsContentProgressModel: HlsContentProgressModel) {
        val view = allViewHolderViews[hlsContentProgressModel.playlistItemId]
        val tvMediaDownloadProgress: AppCompatTextView? =
            view?.findViewById(R.id.tvMediaDownloadProgress)
        val processingProgressBar: CircularProgressIndicator? =
            view?.findViewById(R.id.processing_progress_bar)
        val tvMediaDuration: AppCompatTextView? =
            view?.findViewById(R.id.tvMediaDuration)
        if (hlsContentProgressModel.totalBytes != hlsContentProgressModel.receivedBytes) {
            tvMediaDownloadProgress?.visibility = View.VISIBLE
            processingProgressBar?.visibility = View.VISIBLE
            processingProgressBar?.setProgressCompat(
                hlsContentProgressModel.receivedBytes.toInt(), true
            )
            processingProgressBar?.max = hlsContentProgressModel.totalBytes.toInt()
            tvMediaDownloadProgress?.text =
                view?.resources?.getString(R.string.playlist_preparing_text)
            tvMediaDuration?.visibility = View.GONE
        } else {
            tvMediaDownloadProgress?.visibility = View.GONE
            processingProgressBar?.visibility = View.GONE
        }
    }

    fun updatePlaylistItem(playlistItemModel: PlaylistItemModel) {
        val currentPlaylistItems = ArrayList<PlaylistItemModel>()
        Log.e("updated_item", playlistItemModel.toString())
        currentList.forEach {
            if (it.id == playlistItemModel.id) {
                playlistItemModel.playlistId = it.playlistId
                currentPlaylistItems.add(playlistItemModel)
            } else {
                currentPlaylistItems.add(it)
            }
        }
        submitList(currentPlaylistItems)
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
        val ivMediaThumbnailCurrent: AppCompatImageView? =
            currentPlayingItemView?.findViewById(R.id.ivMediaThumbnail)
        ivMediaThumbnailCurrent?.alpha = 0.4f
        allViewHolderViews.keys.forEach {
            if (it != playlistItemId) {
                val view = allViewHolderViews[it]
                val ivMediaPlayingStatus: AppCompatImageView? =
                    view?.findViewById(R.id.ivMediaPlayingStatus)
                ivMediaPlayingStatus?.visibility = View.GONE
                val mediaTitle: AppCompatTextView? = view?.findViewById(R.id.tvMediaTitle)
                mediaTitle?.setTextColor(view.context.getColor(R.color.playlist_text_color))
                val ivMediaThumbnail: AppCompatImageView? =
                    view?.findViewById(R.id.ivMediaThumbnail)
                ivMediaThumbnail?.alpha = 1.0f
            }
        }
    }
}

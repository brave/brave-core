/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.view

import android.content.Context
import android.content.res.ColorStateList
import android.util.AttributeSet
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import androidx.appcompat.widget.LinearLayoutCompat
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.core.content.withStyledAttributes
import androidx.core.widget.ImageViewCompat
import org.chromium.chrome.browser.playlist.R

class PlaylistToolbar(context: Context, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int) :
    ConstraintLayout(context, attrs, defStyleAttr, defStyleRes) {

    // Main toolbar
    private val layoutMainToolbar: ConstraintLayout
    private val tvTitleToolbarPlaylist: AppCompatTextView
    private val ivOptionsToolbarPlayList: AppCompatImageView
    private val tvActionToolbarPlaylist: AppCompatTextView

    // Edit toolbar
    private val layoutEditToolbar: LinearLayoutCompat
    private val tvItemSelected: AppCompatTextView
    private val ivExitEditMode: AppCompatImageView
    private val ivMoveItem: AppCompatImageView
    private val ivDeleteItem: AppCompatImageView

    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : this(
        context,
        attrs,
        defStyleAttr,
        0
    )

    constructor(context: Context, attrs: AttributeSet?) : this(context, attrs, 0)
    constructor(context: Context) : this(context, null)

    init {
        inflate(context, R.layout.toolbar_playlist, this)
        setBackgroundColor(getColor(android.R.color.transparent))
        layoutMainToolbar = findViewById(R.id.layoutMainToolbar)
        tvTitleToolbarPlaylist = findViewById(R.id.tvTitleToolbarPlaylist)
        val ivBackToolbarPlaylist: AppCompatImageView = findViewById(R.id.ivBackToolbarPlaylist)
        ivOptionsToolbarPlayList = findViewById(R.id.ivOptionsToolbarPlaylist)
        tvActionToolbarPlaylist = findViewById(R.id.tvActionToolbarPlaylist)
        
        context.withStyledAttributes(attrs, R.styleable.PlaylistToolbar) {
            val showOptions = this.getBoolean(R.styleable.PlaylistToolbar_showOptions, false)
            val showCreateButton = this.getBoolean(R.styleable.PlaylistToolbar_showActionButton, false)
            val requireDarkMode = this.getBoolean(R.styleable.PlaylistToolbar_requireDarkMode, false)
            val backButtonIcon = this.getResourceId(
                R.styleable.PlaylistToolbar_backButtonIcon,
                R.drawable.ic_back_toolbar_playlist
            )
            val optionButtonIcon = this.getResourceId(
                R.styleable.PlaylistToolbar_optionButtonIcon,
                R.drawable.ic_options_toolbar_playlist
            )
            val optionButtonTint = this.getResourceId(
                R.styleable.PlaylistToolbar_optionButtonTint,
                android.R.color.white
            )
            val backButtonTint = this.getResourceId(
                R.styleable.PlaylistToolbar_backButtonTint,
                android.R.color.white
            )

            tvTitleToolbarPlaylist.text = this.getString(R.styleable.PlaylistToolbar_title)
            tvActionToolbarPlaylist.text = this.getString(R.styleable.PlaylistToolbar_actionButtonText)

            ivOptionsToolbarPlayList.visibility = if (showOptions) VISIBLE else GONE
            tvActionToolbarPlaylist.visibility = if (showCreateButton) VISIBLE else GONE

            ivOptionsToolbarPlayList.setImageResource(optionButtonIcon)
            ImageViewCompat.setImageTintList(
                ivOptionsToolbarPlayList,
                ColorStateList.valueOf(getColor(optionButtonTint))
            )
            ivBackToolbarPlaylist.setImageResource(backButtonIcon)
            ImageViewCompat.setImageTintList(
                ivBackToolbarPlaylist,
                ColorStateList.valueOf(getColor(backButtonTint))
            )

            if (requireDarkMode) {
                tvTitleToolbarPlaylist.setTextColor(getColor(android.R.color.white))
                ivBackToolbarPlaylist.setColorFilter(getColor(android.R.color.white))
            }
        }
        
        ivBackToolbarPlaylist.setOnClickListener {
            if (context is AppCompatActivity)
                context.onBackPressedDispatcher.onBackPressed()
        }

        layoutEditToolbar = findViewById(R.id.layoutEditToolbar)
        ivExitEditMode = findViewById(R.id.ivExitEditMode)
        tvItemSelected = layoutEditToolbar.findViewById(R.id.tvItemSelected)
        ivMoveItem = layoutEditToolbar.findViewById(R.id.ivMoveItem)
        ivDeleteItem = layoutEditToolbar.findViewById(R.id.ivDeleteItem)
        tvItemSelected.text = context.getString(R.string.playlist_number_selected, 0)
    }

    private fun getColor(color: Int): Int = context.getColor(color)

    fun updateSelectedItems(count: Int) {
        tvItemSelected.text = context.getString(R.string.playlist_number_selected, count)
    }

    fun enableEditMode(enable: Boolean) {
        layoutMainToolbar.visibility = if (enable) GONE else VISIBLE
        layoutEditToolbar.visibility = if (enable) VISIBLE else GONE
        setStatusBarInEditMode()
    }

    private fun setStatusBarInEditMode() {
        if (context is AppCompatActivity) {
            val activity = context as AppCompatActivity
            activity.window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
        }
    }

    fun setActionButtonClickListener(clickListener: OnClickListener) {
        tvActionToolbarPlaylist.setOnClickListener(clickListener)
    }

    fun setExitEditModeClickListener(clickListener: OnClickListener) {
        ivExitEditMode.setOnClickListener(clickListener)
    }

    fun setMoveClickListener(clickListener: OnClickListener) {
        ivMoveItem.setOnClickListener(clickListener)
    }

    fun setDeleteClickListener(clickListener: OnClickListener) {
        ivDeleteItem.setOnClickListener(clickListener)
    }

    fun setToolbarTitle(title: String) {
        tvTitleToolbarPlaylist.text = title
    }

    fun setActionText(actionText: String) {
        tvActionToolbarPlaylist.text = actionText
    }
}

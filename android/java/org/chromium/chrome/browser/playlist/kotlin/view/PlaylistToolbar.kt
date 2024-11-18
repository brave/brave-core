/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.view

import android.content.Context
import android.content.res.ColorStateList
import android.util.AttributeSet
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import androidx.appcompat.widget.LinearLayoutCompat
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.core.widget.ImageViewCompat
import com.brave.playlist.R

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

    private val defaultStatusBarColor: Int

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
        val typedArray = context.obtainStyledAttributes(attrs, R.styleable.PlaylistToolbar)
        val showOptions = typedArray.getBoolean(R.styleable.PlaylistToolbar_showOptions, false)
        val showCreateButton =
            typedArray.getBoolean(R.styleable.PlaylistToolbar_showActionButton, false)
        val requireDarkMode =
            typedArray.getBoolean(R.styleable.PlaylistToolbar_requireDarkMode, false)
        val backButtonIcon = typedArray.getResourceId(
            R.styleable.PlaylistToolbar_backButtonIcon,
            R.drawable.ic_back_toolbar_playlist
        )

        val optionButtonIcon = typedArray.getResourceId(
            R.styleable.PlaylistToolbar_optionButtonIcon,
            R.drawable.ic_options_toolbar_playlist
        )

        val optionButtonTint = typedArray.getResourceId(
            R.styleable.PlaylistToolbar_optionButtonTint,
            android.R.color.white
        )

        val backButtonTint = typedArray.getResourceId(
            R.styleable.PlaylistToolbar_backButtonTint,
            android.R.color.white
        )

        layoutMainToolbar = findViewById(R.id.layoutMainToolbar)
        tvTitleToolbarPlaylist = findViewById(R.id.tvTitleToolbarPlaylist)
        val ivBackToolbarPlaylist: AppCompatImageView = findViewById(R.id.ivBackToolbarPlaylist)
        ivOptionsToolbarPlayList = findViewById(R.id.ivOptionsToolbarPlaylist)
        tvActionToolbarPlaylist = findViewById(R.id.tvActionToolbarPlaylist)
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
        ivOptionsToolbarPlayList.visibility = if (showOptions) VISIBLE else GONE
        tvActionToolbarPlaylist.visibility = if (showCreateButton) VISIBLE else GONE
        ivBackToolbarPlaylist.setOnClickListener {
            if (context is AppCompatActivity)
                context.onBackPressedDispatcher.onBackPressed()
        }
        tvTitleToolbarPlaylist.text = typedArray.getString(R.styleable.PlaylistToolbar_title)
        tvActionToolbarPlaylist.text =
            typedArray.getString(R.styleable.PlaylistToolbar_actionButtonText)

        layoutEditToolbar = findViewById(R.id.layoutEditToolbar)
        ivExitEditMode = findViewById(R.id.ivExitEditMode)
        tvItemSelected = layoutEditToolbar.findViewById(R.id.tvItemSelected)
        ivMoveItem = layoutEditToolbar.findViewById(R.id.ivMoveItem)
        ivDeleteItem = layoutEditToolbar.findViewById(R.id.ivDeleteItem)
        tvItemSelected.text = context.getString(R.string.playlist_number_selected, 0)

        defaultStatusBarColor = if (context is AppCompatActivity)
            context.window.statusBarColor
        else
            0

        if (requireDarkMode) {
            tvTitleToolbarPlaylist.setTextColor(getColor(android.R.color.white))
            ivBackToolbarPlaylist.setColorFilter(getColor(android.R.color.white))
        }

        typedArray.recycle()
    }

    private fun getColor(color: Int): Int = context.getColor(color)

    fun updateSelectedItems(count: Int) {
        tvItemSelected.text = context.getString(R.string.playlist_number_selected, count)
    }

    fun enableEditMode(enable: Boolean) {
        layoutMainToolbar.visibility = if (enable) GONE else VISIBLE
        layoutEditToolbar.visibility = if (enable) VISIBLE else GONE
        setStatusBarInEditMode(enable)
    }

    private fun setStatusBarInEditMode(editMode: Boolean) {
        if (context is AppCompatActivity) {
            val activity = context as AppCompatActivity
            activity.window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
            activity.window.statusBarColor =
                if (editMode) getColor(R.color.edit_toolbar) else defaultStatusBarColor
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

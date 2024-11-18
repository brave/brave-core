/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.util

import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.FragmentActivity
import androidx.transition.Slide
import androidx.transition.TransitionManager
import com.brave.playlist.R
import com.brave.playlist.enums.PlaylistOptionsEnum
import com.brave.playlist.extension.allowMoving
import com.brave.playlist.interpolator.BraveBounceInterpolator
import com.brave.playlist.listener.PlaylistOnboardingActionClickListener
import com.brave.playlist.listener.PlaylistOptionsListener
import com.brave.playlist.model.PlaylistOptionsModel
import com.brave.playlist.model.SnackBarActionModel
import com.brave.playlist.util.PlaylistPreferenceUtils.shouldShowOnboarding
import com.brave.playlist.view.MovableImageButton
import com.brave.playlist.view.PlaylistOnboardingPanel
import com.brave.playlist.view.bottomsheet.PlaylistOptionsBottomSheet
import com.google.android.material.snackbar.Snackbar

@Suppress("unused")
object PlaylistViewUtils {
    @JvmStatic
    fun showPlaylistButton(
        activity: AppCompatActivity,
        parent: ViewGroup,
        playlistOptionsListener: PlaylistOptionsListener,
        playlistOnboardingActionClickListener: PlaylistOnboardingActionClickListener
    ) {
        val movableImageButton = MovableImageButton(activity)
        movableImageButton.id = R.id.playlist_button_id
        movableImageButton.setBackgroundResource(R.drawable.ic_playlist_floating_button_bg)
        movableImageButton.setImageResource(R.drawable.ic_playlist_button)
        val params: FrameLayout.LayoutParams = FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT
        )
        params.marginEnd = 16
        params.bottomMargin = TypedValue.applyDimension(
            TypedValue.COMPLEX_UNIT_DIP,
            40F,
            activity.resources.displayMetrics
        ).toInt()
        params.gravity = Gravity.BOTTOM or Gravity.END
        movableImageButton.layoutParams = params
        movableImageButton.elevation = 8.0f
        movableImageButton.visibility = View.GONE
        movableImageButton.setOnClickListener {
            val shouldShowOnboarding: Boolean =
                PlaylistPreferenceUtils.defaultPrefs(activity).shouldShowOnboarding
            if (shouldShowOnboarding) {
                PlaylistOnboardingPanel(
                    (activity as FragmentActivity),
                    it, playlistOnboardingActionClickListener
                )
                PlaylistPreferenceUtils.defaultPrefs(activity).shouldShowOnboarding = false
            } else {
                PlaylistOptionsBottomSheet(
                    mutableListOf(
                        PlaylistOptionsModel(
                            activity.getString(R.string.playlist_add_media),
                            R.drawable.ic_add_to_media,
                            PlaylistOptionsEnum.ADD_MEDIA
                        ),
                        PlaylistOptionsModel(
                            activity.getString(R.string.playlist_open_playlist),
                            R.drawable.ic_open_playlist,
                            PlaylistOptionsEnum.OPEN_PLAYLIST
                        ),
                        PlaylistOptionsModel(
                            activity.getString(R.string.playlist_open_playlist_settings),
                            R.drawable.ic_playlist_settings,
                            PlaylistOptionsEnum.PLAYLIST_SETTINGS
                        )
                    ), playlistOptionsListener
                ).show((activity as FragmentActivity).supportFragmentManager, null)
            }
        }
        movableImageButton.allowMoving(true)
        if (parent.findViewById<MovableImageButton>(R.id.playlist_button_id) != null) {
            parent.removeView(parent.findViewById<MovableImageButton>(R.id.playlist_button_id))
        }
        parent.addView(movableImageButton)
        val transition = Slide(Gravity.BOTTOM)
            .addTarget(R.id.playlist_button_id)
            .setDuration(500)
            .setInterpolator(BraveBounceInterpolator())

        TransitionManager.beginDelayedTransition(parent, transition)
        movableImageButton.visibility = View.VISIBLE
    }

    @JvmStatic
    fun showSnackBarWithActions(view: View, message: String, action: SnackBarActionModel) {
        val snack = Snackbar.make(view, message, Snackbar.LENGTH_LONG)
        snack.setAction(action.actionText, action.onActionClickListener)
        snack.show()
    }
}

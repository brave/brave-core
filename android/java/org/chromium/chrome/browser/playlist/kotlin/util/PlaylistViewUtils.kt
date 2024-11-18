/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.util

import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.FragmentActivity
import androidx.transition.Slide
import androidx.transition.TransitionManager
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.extension.allowMoving
import org.chromium.chrome.browser.playlist.kotlin.interpolator.BraveBounceInterpolator
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOnboardingActionClickListener
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel
import org.chromium.chrome.browser.playlist.kotlin.model.SnackBarActionModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistModel
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.shouldShowOnboarding
import org.chromium.chrome.browser.playlist.kotlin.view.MovableImageButton
import org.chromium.chrome.browser.playlist.kotlin.view.PlaylistOnboardingPanel
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.PlaylistOptionsBottomSheet
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
                            PlaylistModel.PlaylistOptionsEnum.ADD_MEDIA
                        ),
                        PlaylistOptionsModel(
                            activity.getString(R.string.playlist_open_playlist),
                            R.drawable.ic_open_playlist,
                            PlaylistModel.PlaylistOptionsEnum.OPEN_PLAYLIST
                        ),
                        PlaylistOptionsModel(
                            activity.getString(R.string.playlist_open_playlist_settings),
                            R.drawable.ic_playlist_settings,
                            PlaylistModel.PlaylistOptionsEnum.PLAYLIST_SETTINGS
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

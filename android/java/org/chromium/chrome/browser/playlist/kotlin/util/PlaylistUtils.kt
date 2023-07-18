/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.util

import android.app.Activity
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.net.Uri
import android.os.Build
import android.util.Log
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistMenuOnboardingActivity
import org.chromium.chrome.browser.playlist.kotlin.model.MoveOrCopyModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOnboardingModel
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.PLAYLIST_CHANNEL_ID
import java.util.Date


object PlaylistUtils {
    @JvmStatic
    lateinit var moveOrCopyModel: MoveOrCopyModel
    fun createNotificationChannel(context: Context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                PLAYLIST_CHANNEL_ID,
                context.resources.getString(R.string.playlist_feature_text),
                NotificationManager.IMPORTANCE_HIGH
            )
            serviceChannel.lightColor = Color.BLUE
            serviceChannel.lockscreenVisibility = Notification.VISIBILITY_PRIVATE
            val service =
                context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            service.createNotificationChannel(serviceChannel)
        }
    }

    fun isMediaSourceExpired(mediaSrc: String): Boolean {
        val uri: Uri =
            Uri.parse(mediaSrc)
        if (!uri.getQueryParameter("expire").isNullOrEmpty()) {
            val expireMillis: Long? = uri.getQueryParameter("expire")?.toLong()?.times(1000L)
            return Date() > expireMillis?.let { Date(it) }
        }
        return false
    }

    fun showSharingDialog(context: Context, text: String) {
        val intent = Intent()
        intent.action = Intent.ACTION_SEND
        intent.type = "text/plain"
        intent.putExtra(Intent.EXTRA_TEXT, text)
        context.startActivity(
            Intent.createChooser(
                intent,
                context.resources.getString(R.string.playlist_share_with)
            )
        )
    }

    fun playlistNotificationIntent(
        context: Context,
        playlistItemModel: PlaylistItemModel
    ): Intent? {
        return try {
            val intent = Intent(
                context,
                Class.forName("org.chromium.chrome.browser.playlist.PlaylistHostActivity")
            )
            intent.action = ConstantUtils.PLAYLIST_ACTION
            intent.putExtra(ConstantUtils.CURRENT_PLAYING_ITEM_ID, playlistItemModel.id)
            intent.putExtra(ConstantUtils.CURRENT_PLAYLIST_ID, playlistItemModel.playlistId)
            intent.putExtra(ConstantUtils.PLAYLIST_NAME, playlistItemModel.name)
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        } catch (ex: ClassNotFoundException) {
            Log.e(ConstantUtils.TAG, "playlistNotificationIntent" + ex.message)
            null
        }
    }

    fun getOnboardingItemList(context: Context): List<PlaylistOnboardingModel> {
        return listOf(
            PlaylistOnboardingModel(
                context.getString(R.string.playlist_onboarding_title_1),
                context.getString(R.string.playlist_onboarding_text_1),
                R.drawable.ic_playlist_graphic_1
            ),
            PlaylistOnboardingModel(
                context.getString(R.string.playlist_onboarding_title_2),
                context.getString(R.string.playlist_onboarding_text_2),
                R.drawable.ic_playlist_graphic_2
            ),
            PlaylistOnboardingModel(
                context.getString(R.string.playlist_onboarding_title_3),
                context.getString(R.string.playlist_onboarding_text_3),
                R.drawable.ic_playlist_graphic_3
            )
        )
    }

    @JvmStatic
    fun openPlaylistMenuOnboardingActivity(context: Context) {
        val playlistActivityIntent = Intent(context, PlaylistMenuOnboardingActivity::class.java)
        playlistActivityIntent.flags = Intent.FLAG_ACTIVITY_CLEAR_TOP
        context.startActivity(playlistActivityIntent)
    }

    @JvmStatic
    fun openBraveActivityWithUrl(activity: Activity, url: String) {
        try {
            val intent =
                Intent(activity, Class.forName("org.chromium.chrome.browser.ChromeTabbedActivity"))
            intent.putExtra(ConstantUtils.OPEN_URL, url)
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
            activity.finish()
            activity.startActivity(intent)
        } catch (ex: ClassNotFoundException) {
            Log.e(ConstantUtils.TAG, "openBraveActivityWithUrl : " + ex.message)
        }
    }
}

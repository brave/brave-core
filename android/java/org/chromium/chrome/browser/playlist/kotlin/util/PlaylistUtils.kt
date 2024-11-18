/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.util

import android.app.ActivityManager
import android.content.Context
import android.content.Intent
import android.text.TextUtils
import android.util.Log
import android.util.TypedValue
import androidx.activity.ComponentActivity
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.brave.playlist.R
import com.brave.playlist.activity.PlaylistMenuOnboardingActivity
import com.brave.playlist.local_database.PlaylistRepository
import com.brave.playlist.model.HlsContentProgressModel
import com.brave.playlist.model.MoveOrCopyModel
import com.brave.playlist.model.PlaylistItemModel
import com.brave.playlist.model.PlaylistOnboardingModel

object PlaylistUtils {
    private val TAG: String = "Playlist/"+this::class.java.simpleName

    @JvmStatic
    lateinit var moveOrCopyModel: MoveOrCopyModel

    fun showSharingDialog(context: Context, text: String) {
        val intent = Intent()
        intent.action = Intent.ACTION_SEND
        intent.type = "text/plain"
        intent.putExtra(Intent.EXTRA_TEXT, text)
        context.startActivity(
            Intent.createChooser(
                intent, context.resources.getString(R.string.playlist_share_with)
            )
        )
    }

    fun playlistNotificationIntent(context: Context): Intent? {
        return try {
            val intent = Intent(
                context, Class.forName("org.chromium.chrome.browser.playlist.PlaylistHostActivity")
            )
            intent.action = ConstantUtils.PLAYLIST_ACTION
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        } catch (ex: ClassNotFoundException) {
            Log.e(TAG, "playlistNotificationIntent" + ex.message)
            null
        }
    }

    fun getOnboardingItemList(context: Context): List<PlaylistOnboardingModel> {
        return listOf(
            PlaylistOnboardingModel(
                context.getString(R.string.playlist_onboarding_title_1),
                context.getString(R.string.playlist_onboarding_text_1),
                R.drawable.ic_playlist_graphic_1
            ), PlaylistOnboardingModel(
                context.getString(R.string.playlist_onboarding_title_2),
                context.getString(R.string.playlist_onboarding_text_2),
                R.drawable.ic_playlist_graphic_2
            ), PlaylistOnboardingModel(
                context.getString(R.string.playlist_onboarding_title_3),
                context.getString(R.string.playlist_onboarding_text_3),
                R.drawable.ic_playlist_graphic_3
            )
        )
    }

    @JvmStatic
    @Suppress("unused")
    fun openPlaylistMenuOnboardingActivity(context: Context) {
        val playlistActivityIntent = Intent(context, PlaylistMenuOnboardingActivity::class.java)
        playlistActivityIntent.flags = Intent.FLAG_ACTIVITY_CLEAR_TOP
        context.startActivity(playlistActivityIntent)
    }

    @JvmStatic
    fun openBraveActivityWithUrl(activity: ComponentActivity, url: String) {
        try {
            val intent =
                Intent(activity, Class.forName("org.chromium.chrome.browser.ChromeTabbedActivity"))
            intent.putExtra(ConstantUtils.OPEN_URL, url)
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
            activity.finish()
            activity.startActivity(intent)
        } catch (ex: ClassNotFoundException) {
            Log.e(TAG, "openBraveActivityWithUrl : " + ex.message)
        }
    }

    @JvmStatic
    fun isServiceRunning(context: Context, serviceClass: Class<*>): Boolean {
        val activityManager = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        val runningServices = activityManager.runningAppProcesses

        if (runningServices != null) {
            for (processInfo in runningServices) {
                if (processInfo.processName == serviceClass.name) {
                    return true
                }
            }
        }

        return false
    }

    @JvmStatic
    fun isPlaylistItemCached(selectedPlaylistItemModel: PlaylistItemModel): Boolean {
        return selectedPlaylistItemModel.isCached && (!MediaUtils.isHlsFile(
            selectedPlaylistItemModel.mediaPath
        ) || (MediaUtils.isHlsFile(
            selectedPlaylistItemModel.mediaPath
        ) && !TextUtils.isEmpty(selectedPlaylistItemModel.hlsMediaPath)))
    }

    private val mutableHlsContentProgress = MutableLiveData<HlsContentProgressModel>()
    val hlsContentProgress: LiveData<HlsContentProgressModel> get() = mutableHlsContentProgress

    @JvmStatic
    @Suppress("unused")
    fun updateHlsContentProgress(hlsContentProgressModel: HlsContentProgressModel) {
        mutableHlsContentProgress.value = hlsContentProgressModel
    }

    fun dipToPixels(context: Context, dipValue: Float): Float {
        val metrics = context.resources.displayMetrics
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dipValue, metrics)
    }

    @JvmStatic
    fun checkAndStartHlsDownload(context: Context) {
        try {
            val playlistRepository = PlaylistRepository(context)
            val hlsServiceClass =
                Class.forName("org.chromium.chrome.browser.playlist.hls_content.HlsService")
            if (playlistRepository.getAllHlsContentQueueModel()
                    ?.isNotEmpty() == true && !isServiceRunning(
                    context, hlsServiceClass
                )
            ) {
                    context.startService(
                        Intent(context, hlsServiceClass)
                    )
            }
        } catch (ex: Exception) {
            Log.e(TAG, "hlsServiceClass" + ex.message)
        }
    }
}

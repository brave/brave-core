/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.playback_service

import android.app.PendingIntent
import android.app.TaskStackBuilder
import android.content.Intent
import android.media.session.PlaybackState
import android.os.Handler
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.media3.common.AudioAttributes
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.Player
import androidx.media3.common.util.UnstableApi
import androidx.media3.datasource.DataSource
import androidx.media3.datasource.FileDataSource
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.source.ProgressiveMediaSource
import androidx.media3.session.MediaLibraryService
import androidx.media3.session.MediaSession

import com.google.common.util.concurrent.Futures
import com.google.common.util.concurrent.ListenableFuture

import org.chromium.base.BraveFeatureList
import org.chromium.base.BravePreferenceKeys
import org.chromium.base.ContextUtils
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R
import org.chromium.chrome.browser.flags.ChromeFeatureList
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.playlist.kotlin.util.MediaItemUtil
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.continuousListening
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.rememberFilePlaybackPosition
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.rememberListPlaybackPosition
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistPreferenceUtils.setLatestPlaylistItem
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences
import org.chromium.chrome.browser.profiles.ProfileManager
import org.chromium.mojo.bindings.ConnectionErrorHandler
import org.chromium.mojo.system.MojoException
import org.chromium.playlist.mojom.PlaylistItem
import org.chromium.playlist.mojom.PlaylistService

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

@UnstableApi
class VideoPlaybackService :
    MediaLibraryService(),
    ConnectionErrorHandler,
    MediaLibraryService.MediaLibrarySession.Callback,
    Player.Listener {
    private lateinit var mMediaLibrarySession: MediaLibrarySession
    private val mScope = CoroutineScope(Job() + Dispatchers.IO)
    protected var mPlaylistService: PlaylistService? = null

    companion object {
        private lateinit var mPlayer: ExoPlayer
        var currentPlaylistId: String = ""

        private val mutableCurrentPlayingItem = MutableLiveData<String>()
        val currentPlayingItem: LiveData<String>
            get() = mutableCurrentPlayingItem

        private fun setCurrentPlayingItem(currentPlayingItemId: String) {
            mutableCurrentPlayingItem.value = currentPlayingItemId
        }

        private var mediaItemsInPlayer: ArrayList<MediaItem> = ArrayList()

        fun addNewPlaylistItemModel(newPlaylistItem: PlaylistItem) {
            if (ConstantUtils.DEFAULT_PLAYLIST == currentPlaylistId) {
                val mediaItem =
                    MediaItemUtil.buildMediaItem(
                        newPlaylistItem,
                        ConstantUtils.DEFAULT_PLAYLIST,
                        ContextUtils.getApplicationContext()
                            .resources
                            .getString(
                                R.string.playlist_play_later
                            ),
                    )
                mediaItemsInPlayer.add(mediaItem)
                mPlayer.addMediaItem(mediaItem)
            }
        }


        fun removePlaylistItemModel(playlistItemId: String) {
            mediaItemsInPlayer.forEachIndexed { index, mediaItem ->
                if (mediaItem.mediaId == playlistItemId && mediaItem.mediaId != currentPlaylistId) {
                    mPlayer.removeMediaItem(index)
                    mediaItemsInPlayer.removeAt(index)
                }
            }
        }

        fun reorderPlaylistItemModel(playlistItemList: List<PlaylistItem>) {
            mediaItemsInPlayer.forEachIndexed { oldIndex, mediaItem ->
                val newIndex = playlistItemList.indexOfFirst { it.id == mediaItem.mediaId }
                mPlayer.moveMediaItem(oldIndex, newIndex)
            }
        }

        fun getMediaItemIndex(playlistItemId: String): Int {
            return mediaItemsInPlayer.indexOfFirst { it.mediaId == playlistItemId }
        }
    }

    override fun onCreate() {
        super.onCreate()
        initPlaylistService()
        initializeSessionAndPlayer()
        lastSavedPositionTimer()
    }

    override fun onConnectionError(mojoException: MojoException) {
        mPlaylistService?.close()
        mPlaylistService = null
        if (
            ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST) &&
                ChromeSharedPreferences.getInstance()
                    .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)
        ) {
            initPlaylistService()
        }
    }

    private fun initPlaylistService() {
        if (mPlaylistService != null) {
            mPlaylistService = null
        }
        mPlaylistService =
            PlaylistServiceFactoryAndroid.getInstance()
                .getPlaylistService(ProfileManager.getLastUsedRegularProfile(), this)
    }

    private fun initializeSessionAndPlayer() {
        val dataSourceFactory = DataSource.Factory { FileDataSource.Factory().createDataSource() }
        val mediaSourceFactory = ProgressiveMediaSource.Factory(dataSourceFactory)
        mPlayer =
            ExoPlayer.Builder(this)
                .setMediaSourceFactory(mediaSourceFactory)
                .setHandleAudioBecomingNoisy(true)
                .setWakeMode(C.WAKE_MODE_LOCAL)
                .setAudioAttributes(AudioAttributes.DEFAULT, true)
                .build()
        mPlayer.addListener(this)

        mMediaLibrarySession =
            MediaLibrarySession.Builder(this, mPlayer, this)
                .setSessionActivity(buildPendingIntent())
                .build()
    }

    private fun buildPendingIntent(): PendingIntent {
        val intent = PlaylistUtils.playlistNotificationIntent(applicationContext)
        val pendingIntent =
            TaskStackBuilder.create(this).run {
                addNextIntent(intent)
                getPendingIntent(
                    0,
                    PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
                )
            }
        return pendingIntent
    }

    override fun onGetSession(controllerInfo: MediaSession.ControllerInfo): MediaLibrarySession {
        return mMediaLibrarySession
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
        if (!mPlayer.playWhenReady || mPlayer.mediaItemCount == 0) {
            stopSelf()
        }
    }

    override fun onDestroy() {
        mMediaLibrarySession.release()
        mPlayer.release()
        clearListener()
        cancelLastSavedPositionTimer()
        mPlaylistService?.close()
        mPlaylistService = null
        super.onDestroy()
    }

    override fun onAddMediaItems(
        mediaSession: MediaSession,
        controller: MediaSession.ControllerInfo,
        mediaItems: List<MediaItem>
    ): ListenableFuture<List<MediaItem>> {
        // We need to use URI from requestMetaData because of
        // https://github.com/androidx/media/issues/282
        val updatedMediaItems: List<MediaItem> =
            mediaItems.map { mediaItem ->
                MediaItem.Builder()
                    .setMediaId(mediaItem.mediaId)
                    .setRequestMetadata(mediaItem.requestMetadata)
                    .setMediaMetadata(mediaItem.mediaMetadata)
                    .setUri(mediaItem.requestMetadata.mediaUri)
                    .build()
            }
        mediaItemsInPlayer.clear()
        mediaItemsInPlayer.addAll(updatedMediaItems)
        return Futures.immediateFuture(updatedMediaItems)
    }

    // Last saved position timer
    private var mLastSavedPositionHandler: Handler? = null
    private val mSavePositionRunnableCode: Runnable =
        object : Runnable {
            override fun run() {
                if (mPlayer.isPlaying) {
                    mPlayer.currentMediaItem?.let { saveLastPosition(it, mPlayer.currentPosition) }
                }
                mLastSavedPositionHandler?.postDelayed(this, 2000)
            }
        }

    private fun lastSavedPositionTimer() {
        mLastSavedPositionHandler = Handler(mPlayer.applicationLooper)
        mLastSavedPositionHandler?.post(mSavePositionRunnableCode)
    }

    private fun cancelLastSavedPositionTimer() {
        mLastSavedPositionHandler?.removeCallbacks(mSavePositionRunnableCode)
    }

    // Player callbacks
    override fun onPlaybackStateChanged(playbackState: @Player.State Int) {
        if (playbackState == Player.STATE_ENDED) {
            mPlayer.currentMediaItem?.let { saveLastPosition(it, 0) }
        }
    }

    override fun onMediaItemTransition(mediaItem: MediaItem?, reason: Int) {
        super.onMediaItemTransition(mediaItem, reason)
        if (reason != Player.MEDIA_ITEM_TRANSITION_REASON_SEEK) {
            mPlayer.playWhenReady =
                PlaylistPreferenceUtils.defaultPrefs(applicationContext).continuousListening
        }
    }

    override fun onPositionDiscontinuity(
        oldPosition: Player.PositionInfo,
        newPosition: Player.PositionInfo,
        reason: @Player.DiscontinuityReason Int
    ) {
        val playbackState = mPlayer.playbackState
        if (playbackState == PlaybackState.STATE_PLAYING) {
            val size = mPlayer.mediaItemCount
            val previousItemIndex =
                if (mPlayer.currentMediaItemIndex in 1 until size)
                    (mPlayer.currentMediaItemIndex - 1)
                else mPlayer.currentMediaItemIndex
            saveLastPosition(mPlayer.getMediaItemAt(previousItemIndex), 0)
        }
    }

    private fun saveLastPosition(
        mediaItem: MediaItem,
        @Suppress("UNUSED_PARAMETER") currentPosition: Long
    ) {
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK) {
            if (PlaylistPreferenceUtils.defaultPrefs(applicationContext).rememberFilePlaybackPosition) {
                mediaItem.mediaId.let { mediaId ->
                    mPlaylistService?.updateItemLastPlayedPosition(mediaId, currentPosition.toInt())
                }
            }
        }
        if (PlaylistPreferenceUtils.defaultPrefs(applicationContext).rememberListPlaybackPosition) {
            mediaItem.let {
                val playlistId = it.mediaMetadata.extras?.getString(ConstantUtils.PLAYLIST_ID) ?: ""
                PlaylistPreferenceUtils.defaultPrefs(applicationContext)
                    .setLatestPlaylistItem(playlistId, it.mediaId)
            }
        }
        updateCurrentlyPlayedItem()
    }

    private fun updateCurrentlyPlayedItem() {
        mPlayer.currentMediaItem?.let {
            setCurrentPlayingItem(it.mediaId)
            currentPlaylistId = it.mediaMetadata.extras?.getString(ConstantUtils.PLAYLIST_ID) ?: ""
        }
    }
}

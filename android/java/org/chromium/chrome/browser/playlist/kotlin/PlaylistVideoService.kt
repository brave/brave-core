/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin

import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.drawable.Drawable
import android.media.session.PlaybackState
import android.net.Uri
import android.os.Binder
import android.os.Handler
import android.os.IBinder
import android.os.Build
import android.support.v4.media.session.MediaSessionCompat
import android.util.Log
import androidx.lifecycle.LiveData
import org.chromium.chrome.R
import androidx.lifecycle.MutableLiveData
import com.brave.playlist.local_database.PlaylistRepository
import com.brave.playlist.model.PlaylistItemModel
import com.brave.playlist.util.ConstantUtils
import com.brave.playlist.util.ConstantUtils.PLAYER_ITEMS
import com.brave.playlist.util.ConstantUtils.PLAYLIST_NAME
import com.brave.playlist.util.ConstantUtils.TAG
import com.brave.playlist.util.PlaylistPreferenceUtils
import com.brave.playlist.util.PlaylistPreferenceUtils.continuousListening
import com.brave.playlist.util.PlaylistPreferenceUtils.rememberFilePlaybackPosition
import com.brave.playlist.util.PlaylistPreferenceUtils.rememberListPlaybackPosition
import com.brave.playlist.util.PlaylistPreferenceUtils.setLatestPlaylistItem
import com.brave.playlist.util.PlaylistUtils
import com.brave.playlist.util.PlaylistUtils.createNotificationChannel
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker
import com.bumptech.glide.Glide
import com.bumptech.glide.request.target.CustomTarget
import com.bumptech.glide.request.transition.Transition
import com.google.android.exoplayer2.C
import com.google.android.exoplayer2.DefaultLoadControl
import com.google.android.exoplayer2.ExoPlayer
import com.google.android.exoplayer2.MediaItem
import com.google.android.exoplayer2.MediaMetadata
import com.google.android.exoplayer2.PlaybackException
import com.google.android.exoplayer2.Player
import com.google.android.exoplayer2.Timeline
import com.google.android.exoplayer2.audio.AudioAttributes
import com.google.android.exoplayer2.ext.cast.CastPlayer
import com.google.android.exoplayer2.ext.cast.SessionAvailabilityListener
import com.google.android.exoplayer2.ext.mediasession.MediaSessionConnector
import com.google.android.exoplayer2.source.DefaultMediaSourceFactory
import com.google.android.exoplayer2.ui.PlayerNotificationManager
import com.google.android.exoplayer2.ui.StyledPlayerControlView
import com.google.android.exoplayer2.ui.StyledPlayerView
import com.google.android.exoplayer2.util.MimeTypes
import com.google.android.gms.cast.framework.CastContext
import org.chromium.components.media_router.caf.CastUtils
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import org.chromium.chrome.browser.vpn.BraveVpnObserver

class PlaylistVideoService : Service(), Player.Listener, SessionAvailabilityListener, BraveVpnObserver {
    private var mPlaylistName: String? = null
    private var mPlaylistItemsModel: ArrayList<PlaylistItemModel>? = arrayListOf()
    private var mPlayerNotificationManager: PlayerNotificationManager? = null
    private var mLocalPlayer: ExoPlayer? = null
    private var mCastPlayer: CastPlayer? = null
    private var mCastContext: CastContext? = null
    private val mMediaQueue: ArrayList<MediaItem> = ArrayList()
    private val mCastMediaQueue: ArrayList<MediaItem> = ArrayList()
    private var mPlayerView: StyledPlayerView? = null
    private var mMediaSessionConnector: MediaSessionConnector? = null

    private val mScope = CoroutineScope(Job() + Dispatchers.IO)

    private var mCurrentItemIndex: Int = 0
    private var mCurrentPlayer: Player? = null

    private var mLastSavedPositionHandler: Handler? = null
    private val mPlaylistRepository: PlaylistRepository by lazy {
        PlaylistRepository(applicationContext)
    }
    private val mSavePositionRunnableCode: Runnable = object : Runnable {
        override fun run() {
            if (mCurrentPlayer?.isPlaying == true) {
                Log.e(TAG, "runnableCode")
                val currentPosition = mCurrentPlayer?.currentPosition
                currentPosition?.let { saveLastPosition(mCurrentItemIndex, it) }
            }
            mLastSavedPositionHandler?.postDelayed(this, 5000)
        }
    }

    companion object {
        var CURRENTLY_PLAYED_ITEM_ID: String? = null
        private val mutableCastStatus = MutableLiveData<Boolean>()
        val castStatus: LiveData<Boolean> get() = mutableCastStatus
        private fun setCastStatus(shouldShowControls: Boolean) {
            mutableCastStatus.value = shouldShowControls
        }

        private val mutableCurrentPlayingItem = MutableLiveData<String>()
        val currentPlayingItem: LiveData<String> get() = mutableCurrentPlayingItem
        private fun setCurrentPlayingItem(currentPlayingItemId: String) {
            mutableCurrentPlayingItem.value = currentPlayingItemId
        }
    }

    private fun lastSavedPositionTimer() {
        mLastSavedPositionHandler = mCurrentPlayer?.applicationLooper?.let { Handler(it) }
        mLastSavedPositionHandler?.post(mSavePositionRunnableCode)
    }

    private fun cancelLastSavedPositionTimer() {
        mLastSavedPositionHandler?.removeCallbacks(mSavePositionRunnableCode)
    }

    fun setPlayerView(styledPlayerView: StyledPlayerView) {
        mPlayerView = styledPlayerView
    }

    override fun onBind(intent: Intent?): IBinder {
        Log.e(TAG, "onBind")
        return PlaylistVideoServiceBinder()
    }

    override fun onUnbind(intent: Intent?): Boolean {
        return super.onUnbind(intent)
    }

    // override fun onDataReceived(response:ByteArray) {
    //     Log.e("data_source", "PlaylistVideoService");
    //     mCurrentPlayer?.prepare()
    //     mCurrentPlayer?.playWhenReady = true
    // }

    override fun onCreate() {
        super.onCreate()

        createNotificationChannel(applicationContext)
        mCurrentItemIndex = C.INDEX_UNSET
        val loadControl = DefaultLoadControl.Builder()
            .setBufferDurationsMs(32 * 1024, 64 * 1024, 1024, 1024)
            .build()
        val audioAttributes: AudioAttributes = AudioAttributes.Builder()
            .setUsage(C.USAGE_MEDIA)
            .setContentType(C.CONTENT_TYPE_MOVIE)
            .build()
        mLocalPlayer = ExoPlayer.Builder(applicationContext)
            .setMediaSourceFactory(
                DefaultMediaSourceFactory(
                    PlaylistDownloadUtils.getDataSourceFactory(
                        applicationContext
                    )
                )
            )
            .setLoadControl(loadControl)
            .setReleaseTimeoutMs(5000).setAudioAttributes(audioAttributes, true).build()
        mLocalPlayer?.videoScalingMode = C.VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING
        mLocalPlayer?.addListener(this)
        // mCastContext = CastContext.getSharedInstance()
        mCastContext = CastUtils.getCastContext();
        mCastPlayer = mCastContext?.let { CastPlayer(it) }
        mCastPlayer?.addListener(this)
        mCastPlayer?.setSessionAvailabilityListener(this)
    }

    private fun release() {
        CURRENTLY_PLAYED_ITEM_ID = null
        mCurrentItemIndex = C.INDEX_UNSET
        mMediaQueue.clear()
        mCastMediaQueue.clear()
        mPlayerNotificationManager?.setPlayer(null)
        mCastPlayer?.setSessionAvailabilityListener(null)
        mCastPlayer?.release()
        mPlayerView?.player = null
        mLocalPlayer?.release()
    }

    fun getCurrentPlayer() = mCurrentPlayer

    override fun onDestroy() {
        mPlaylistName = null
        mPlaylistItemsModel = null
        mMediaSessionConnector = null
        release()
        cancelLastSavedPositionTimer()
        super.onDestroy()
    }

    @Suppress("DEPRECATION")
    private fun getPlaylistItemModels(intent: Intent): ArrayList<PlaylistItemModel>? {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            intent.getParcelableArrayListExtra(
                PLAYER_ITEMS,
                PlaylistItemModel::class.java
            )
        } else {
            intent.getParcelableArrayListExtra(PLAYER_ITEMS)
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.e(TAG, "onStartCommand")
        BraveVpnNativeWorker.getInstance().addObserver(this@PlaylistVideoService);
        intent?.let {
            mPlaylistName = it.getStringExtra(PLAYLIST_NAME)
            mPlaylistItemsModel = getPlaylistItemModels(it)
        }

        mPlayerNotificationManager = PlayerNotificationManager.Builder(
            applicationContext,
            ConstantUtils.PLAYLIST_NOTIFICATION_ID,
            ConstantUtils.PLAYLIST_CHANNEL_ID
        ).setMediaDescriptionAdapter(object : PlayerNotificationManager.MediaDescriptionAdapter {
            override fun getCurrentContentTitle(player: Player): CharSequence {
                return mPlaylistName.toString()
            }

            override fun createCurrentContentIntent(player: Player): PendingIntent? {
                return PendingIntent.getActivity(
                    applicationContext,
                    0,
                    getMediaItemFromPosition(player.currentMediaItemIndex)
                        ?.let { PlaylistUtils.playlistNotificationIntent(applicationContext, it) },
                    PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
                )
            }

            override fun getCurrentContentText(player: Player): CharSequence {
                return getMediaItemFromPosition(player.currentMediaItemIndex)?.name ?: ""
            }

            override fun getCurrentLargeIcon(
                player: Player,
                callback: PlayerNotificationManager.BitmapCallback
            ): Bitmap? {
                Glide.with(applicationContext)
                    .asBitmap()
                    .load(getMediaItemFromPosition(player.currentMediaItemIndex)?.thumbnailPath)
                    .into(object : CustomTarget<Bitmap?>() {
                        override fun onResourceReady(
                            resource: Bitmap,
                            transition: Transition<in Bitmap?>?
                        ) {
                            callback.onBitmap(resource)
                        }

                        override fun onLoadCleared(placeholder: Drawable?) {}
                    })
                return null
            }
        }).setSmallIconResourceId(R.drawable.ic_playing_sound)
            .build()

        Log.e(TAG, "playlistItemsModel?.size : " + mPlaylistItemsModel?.size.toString())
        Log.e(TAG, "playlistItemsModel : " + mPlaylistItemsModel.toString())
        mMediaQueue.clear()
        mCastMediaQueue.clear()
        mPlaylistItemsModel?.forEach { mediaModel ->
            val movieMetadata: MediaMetadata =
                MediaMetadata.Builder().setTitle(mediaModel.name).setArtist(mediaModel.author)
                    .setArtworkUri(Uri.parse(mediaModel.thumbnailPath)).build()
            val onlineMediaItem = MediaItem.Builder()
                .setUri(Uri.parse(if (mediaModel.isCached) mediaModel.mediaPath else mediaModel.mediaSrc))
                .setMediaMetadata(movieMetadata)
                .build()
            val mediaItem: MediaItem = PlaylistDownloadUtils.getMediaItemFromDownloadRequest(
                applicationContext,
                mediaModel
            ) ?: onlineMediaItem
            val castMediaItem = MediaItem.Builder()
//                .setUri("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4")
                .setUri(mediaModel.mediaSrc)
                .setMediaMetadata(movieMetadata)
                .setMimeType(MimeTypes.VIDEO_MP4)
                .build()
            mMediaQueue.add(mediaItem)
            mCastMediaQueue.add(castMediaItem)
        }

        Log.e(TAG, "onStartCommand : setCurrentPlayer")
        setCurrentPlayer(if (mCastPlayer?.isCastSessionAvailable == true) mCastPlayer else mLocalPlayer)
        mScope.launch {
            lastSavedPositionTimer()
        }
        return START_STICKY
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
        super.onTaskRemoved(rootIntent)
        val notificationManager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.cancel(ConstantUtils.PLAYLIST_NOTIFICATION_ID)
    }

    inner class PlaylistVideoServiceBinder : Binder() {
        fun getServiceInstance() = this@PlaylistVideoService
    }

    private fun getCurrentPlayingItem() = mCurrentPlayer?.currentMediaItemIndex?.let {
        getMediaItemFromPosition(
            it
        )
    }

    // Player.Listener implementation.
    override fun onPlaybackStateChanged(playbackState: @Player.State Int) {
        if (playbackState == Player.STATE_ENDED) {
            Log.e(TAG, "onPlaybackStateChanged")
            saveLastPosition(mCurrentItemIndex, 0)
        }
        updateCurrentItemIndex()
    }

    override fun onPlayerError(error: PlaybackException) {
        super.onPlayerError(error)
//         when (error.errorCode) {
//             PlaybackException.ERROR_CODE_PARSING_CONTAINER_UNSUPPORTED -> {
//                 val currentPlaylistItemModel = getCurrentPlayingItem()
//                 val movieMetadata: MediaMetadata =
//                     MediaMetadata.Builder().setTitle(currentPlaylistItemModel?.name)
//                         .setArtist(currentPlaylistItemModel?.author)
//                         .setArtworkUri(Uri.parse(currentPlaylistItemModel?.thumbnailPath)).build()
//                 val mediaItem = MediaItem.Builder()
// //                .setUri("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4")
//                     .setUri(Uri.parse(currentPlaylistItemModel?.mediaSrc))
//                     .setMediaMetadata(movieMetadata)
//                     .setMimeType(MimeTypes.APPLICATION_M3U8)
//                     .build()
//                 mMediaQueue[mCurrentPlayer?.currentMediaItemIndex ?: 0] = mediaItem
//                 mCastMediaQueue[mCurrentPlayer?.currentMediaItemIndex ?: 0] = mediaItem
//                 Log.e(TAG, "onPlayerError : setCurrentPlayer")
//                 setCurrentPlayer(if (mCastPlayer?.isCastSessionAvailable == true) mCastPlayer else mLocalPlayer)
//             }

//             else -> {
//                 if (mCurrentPlayer?.hasNextMediaItem() == true) {
//                     mCurrentPlayer?.nextMediaItemIndex?.let { setCurrentItem(it) }
//                 }
//             }
//         }
        Log.e(TAG, "onPlayerError : " + error.message.toString())
    }

    override fun onMediaItemTransition(mediaItem: MediaItem?, reason: Int) {
        super.onMediaItemTransition(mediaItem, reason)
        Log.e(
            TAG,
            PlaylistVideoService::class.java.name + " : onMediaItemTransition : Reason : " + reason
        )
        if (reason != Player.MEDIA_ITEM_TRANSITION_REASON_SEEK) {
            mCurrentPlayer?.playWhenReady =
                PlaylistPreferenceUtils.defaultPrefs(applicationContext).continuousListening
        }
    }

    override fun onPositionDiscontinuity(
        oldPosition: Player.PositionInfo,
        newPosition: Player.PositionInfo,
        reason: @Player.DiscontinuityReason Int
    ) {
        updateCurrentItemIndex()
        val playbackState = mCurrentPlayer?.playbackState
        if (playbackState == PlaybackState.STATE_PLAYING) {
            Log.e(TAG, "onPositionDiscontinuity")
            val size = mPlaylistItemsModel?.size ?: 0
            val previousItemIndex =
                if (mCurrentItemIndex in 1 until size) mCurrentItemIndex - 1 else mCurrentItemIndex
            saveLastPosition(previousItemIndex, 0)
            sendCurrentPlayingItemBroadcast()
        }
    }

    private fun saveLastPosition(itemIndex: Int, currentPosition: Long) {
        mScope.launch {
            getMediaItemFromPosition(itemIndex)
                ?.let {
                    it.lastPlayedPosition = currentPosition
                    if (PlaylistPreferenceUtils.defaultPrefs(applicationContext).rememberFilePlaybackPosition) {
                        mPlaylistRepository.insertPlaylistItemModel(it)
                    }
                    if (PlaylistPreferenceUtils.defaultPrefs(applicationContext).rememberListPlaybackPosition) {
                        PlaylistPreferenceUtils.defaultPrefs(applicationContext)
                            .setLatestPlaylistItem(it.playlistId, it.id)
                    }
                }
        }
        sendCurrentPlayingItemBroadcast()
    }

    override fun onTimelineChanged(timeline: Timeline, reason: @Player.TimelineChangeReason Int) {
        updateCurrentItemIndex()
    }

    // CastPlayer.SessionAvailabilityListener implementation.
    override fun onCastSessionAvailable() {
        Log.e(TAG, "onCastSessionUnavailable : setCurrentPlayer")
        setCurrentPlayer(mCastPlayer)
        setCastStatus(false)
    }

    override fun onCastSessionUnavailable() {
        Log.e(TAG, "onCastSessionUnavailable : setCurrentPlayer")
        setCurrentPlayer(mLocalPlayer)
        setCastStatus(true)
    }

    private fun sendCurrentPlayingItemBroadcast() {
        Log.e("NTP", "sendCurrentPlayingItemBroadcast")
        val currentPlayingItemId = mCurrentPlayer?.currentMediaItemIndex?.let {
            getMediaItemFromPosition(
                it
            )?.id
        }
        if (!currentPlayingItemId.isNullOrEmpty()) {
            setCurrentPlayingItem(currentPlayingItemId)
        }
        CURRENTLY_PLAYED_ITEM_ID = currentPlayingItemId
        Log.e("CURRENTLY_PLAYED_ITEM_ID", CURRENTLY_PLAYED_ITEM_ID.toString())
    }

    private fun getMediaItemFromPosition(position: Int): PlaylistItemModel? {
//        if ((playlistItemsModel?.size ?: 0) >= position) {
//            return null
//        }
        return mPlaylistItemsModel?.get(position)
    }

    // Internal methods.
    private fun updateCurrentItemIndex() {
        val playbackState = mCurrentPlayer?.playbackState
        maybeSetCurrentItemAndNotify(
            if (playbackState != Player.STATE_IDLE && playbackState != Player.STATE_ENDED) mCurrentPlayer?.currentMediaItemIndex
                ?: C.INDEX_UNSET else C.INDEX_UNSET
        )
    }

    private fun setCurrentPlayer(currentPlayer: Player?) {
//        if (this.currentPlayer === currentPlayer) {
//            return
//        }
        mPlayerView?.player = currentPlayer
        mPlayerView?.controllerHideOnTouch = currentPlayer === mLocalPlayer
        if (currentPlayer === mCastPlayer) {
            mPlayerView?.controllerShowTimeoutMs = 0
            mPlayerView?.useController = true
            mPlayerView?.controllerHideOnTouch = false
            mPlayerView?.showController()
        } else { // currentPlayer == localPlayer
            mPlayerView?.useController = false
            val mediaSession = MediaSessionCompat(applicationContext, TAG)
            mMediaSessionConnector = MediaSessionConnector(mediaSession)
            mMediaSessionConnector?.setPlayer(currentPlayer)
            mediaSession.isActive = true
            mPlayerNotificationManager?.setPlayer(currentPlayer)
            mPlayerNotificationManager?.setMediaSessionToken(mediaSession.sessionToken)
            mPlayerView?.controllerShowTimeoutMs = StyledPlayerControlView.DEFAULT_SHOW_TIMEOUT_MS
//            val audioAttributes: AudioAttributes = AudioAttributes.Builder()
//                .setUsage(C.USAGE_MEDIA)
//                .setContentType(C.AUDIO_CONTENT_TYPE_MOVIE)
//                .build()
//
//            localPlayer?.setAudioAttributes(audioAttributes, true)
        }

        // Player state management.
        var playbackPositionMs = C.TIME_UNSET
        var currentItemIndex = C.INDEX_UNSET
        var playWhenReady = true
        val previousPlayer = this.mCurrentPlayer
        if (previousPlayer != null) {
            // Save state from the previous player.
            val playbackState = previousPlayer.playbackState
            if (playbackState != Player.STATE_ENDED) {
                playbackPositionMs = previousPlayer.currentPosition
                playWhenReady = previousPlayer.playWhenReady
                currentItemIndex = previousPlayer.currentMediaItemIndex
                if (currentItemIndex != this.mCurrentItemIndex) {
                    playbackPositionMs = C.TIME_UNSET
                    currentItemIndex = this.mCurrentItemIndex
                }
            }
            previousPlayer.stop()
            previousPlayer.clearMediaItems()
        }
        this.mCurrentPlayer = currentPlayer

        Log.e(TAG, "before currentPlayer?.mediaItemCount : " + currentPlayer?.mediaItemCount)
        // Media queue management.
        if (currentPlayer === mCastPlayer) {
            currentPlayer?.setMediaItems(mCastMediaQueue, currentItemIndex, playbackPositionMs)
        } else {
            currentPlayer?.setMediaItems(mMediaQueue, currentItemIndex, playbackPositionMs)
        }
        Log.e(TAG, "after currentPlayer?.mediaItemCount : " + currentPlayer?.mediaItemCount)
        currentPlayer?.playWhenReady = playWhenReady
        currentPlayer?.prepare()
        currentPlayer?.play()
    }

    fun setCurrentItem(itemIndex: Int) {
        maybeSetCurrentItemAndNotify(itemIndex)
        if (mCurrentPlayer?.currentTimeline?.windowCount != mMediaQueue.size) {
            // This only happens with the cast player. The receiver app in the cast device clears the
            // timeline when the last item of the timeline has been played to end.
            if (mCurrentPlayer === mCastPlayer) {
                mCurrentPlayer?.setMediaItems(mCastMediaQueue, itemIndex, C.TIME_UNSET)
            } else {
                mCurrentPlayer?.setMediaItems(mMediaQueue, itemIndex, C.TIME_UNSET)
            }
        } else {
            mPlaylistItemsModel?.get(mCurrentItemIndex)?.lastPlayedPosition
                ?.let { mCurrentPlayer?.seekTo(itemIndex, it) }
        }
        mCurrentPlayer?.playWhenReady = true
    }

    private fun maybeSetCurrentItemAndNotify(currentItemIndex: Int) {
        if (this.mCurrentItemIndex != currentItemIndex) {
            this.mCurrentItemIndex = currentItemIndex
            CURRENTLY_PLAYED_ITEM_ID = getCurrentPlayingItem()?.id
            Log.e(TAG, "CURRENTLY_PLAYED_ITEM_ID: " + CURRENTLY_PLAYED_ITEM_ID.toString())
        }
    }
}

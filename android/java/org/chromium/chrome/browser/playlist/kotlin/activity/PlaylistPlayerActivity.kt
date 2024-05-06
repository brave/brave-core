/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.annotation.SuppressLint
import android.content.*
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.os.*
import org.chromium.base.Log
import android.provider.Settings
import android.util.TypedValue
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.ProgressBar
import android.widget.SeekBar
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import androidx.appcompat.widget.LinearLayoutCompat
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.media3.common.MediaItem
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.session.MediaController
import androidx.media3.session.SessionToken
import androidx.media3.ui.AspectRatioFrameLayout
import androidx.media3.ui.PlayerView
import androidx.mediarouter.app.MediaRouteButton
import androidx.recyclerview.widget.RecyclerView
import org.chromium.chrome.browser.playlist.kotlin.PlaylistViewModel
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.playlist.mojom.PlaylistEvent
import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview.PlaylistItemAdapter
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.chrome.browser.playlist.kotlin.extension.afterMeasured
import org.chromium.chrome.browser.playlist.kotlin.extension.dpToPx
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemClickListener
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.*
import org.chromium.chrome.browser.playlist.kotlin.playback_service.VideoPlaybackService
import org.chromium.chrome.browser.playlist.kotlin.slidingpanel.BottomPanelLayout
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.DEFAULT_PLAYLIST
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.PLAYLIST_MODEL
import org.chromium.chrome.browser.playlist.kotlin.util.MenuUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import org.chromium.chrome.browser.playlist.kotlin.view.PlaylistToolbar
import com.google.android.gms.cast.framework.CastButtonFactory
import com.google.android.material.card.MaterialCardView
import com.google.common.util.concurrent.ListenableFuture
import com.google.common.util.concurrent.MoreExecutors
import org.chromium.chrome.browser.playlist.kotlin.activity.PlaylistBaseActivity
import org.chromium.playlist.mojom.Playlist
import org.chromium.playlist.mojom.PlaylistItem

class PlaylistPlayerActivity : PlaylistBaseActivity(), Player.Listener, BottomPanelLayout.PanelSlideListener, PlaylistItemClickListener  {
    companion object {
        val TAG: String = "PlaylistPlayerActivity"
        private const val SEEK_VALUE_MS = 15000
    }

    private lateinit var mPlaylist: Playlist

    private var mIsCastInProgress: Boolean = false
    private var mDuration: Long = 0
    private var mIsUserTrackingTouch = false
    private var mCurrentMediaIndex = 0
    private var mPlaybackPosition = 0L
    private var mIsShuffleOn = false
    private var mPlayWhenReady = true
    private var mPlaybackSpeed = 1f
    private var mRepeatMode = Player.REPEAT_MODE_OFF
    private var mUpdatePositionDelayMs = 1000L

    private lateinit var mPlaylistToolbar: PlaylistToolbar
    private lateinit var mPlayerView: PlayerView
    private lateinit var mAspectRatioFrameLayout: AspectRatioFrameLayout
    private lateinit var mHoverControlsLayout: LinearLayoutCompat
    private lateinit var mEmptyView: View
    private lateinit var mFullscreenImg: AppCompatImageView
    private lateinit var mBackImg: AppCompatImageView
    private lateinit var mVideoSeekBar: SeekBar
    private lateinit var mTvVideoTitle: AppCompatTextView
    private lateinit var mTvVideoSource: AppCompatTextView
    private lateinit var mTvVideoTimeElapsed: AppCompatTextView
    private lateinit var mTvVideoTimeRemaining: AppCompatTextView
    private lateinit var mIvPlaylistMediaSpeed: AppCompatImageView
    private lateinit var mIvPlaylistRepeat: AppCompatImageView
    private lateinit var mIvPlaylistShuffle: AppCompatImageView
    private lateinit var mIvNextVideo: AppCompatImageView
    private lateinit var mIvPrevVideo: AppCompatImageView
    private lateinit var mIvPlayPauseVideo: AppCompatImageView
    private lateinit var mIvSeekForward15Seconds: AppCompatImageView
    private lateinit var mIvSeekBack15Seconds: AppCompatImageView
    private lateinit var mLayoutVideoControls: ConstraintLayout
    private lateinit var mLayoutBottom: MaterialCardView
    private lateinit var mLayoutPlayer: LinearLayoutCompat
    private lateinit var mIvVideoOptions: AppCompatImageView
    private lateinit var mTvPlaylistName: AppCompatTextView
    private lateinit var mProgressBar: ProgressBar

    private lateinit var mVideoPlayerLoading: ProgressBar

    private lateinit var mRvPlaylist: RecyclerView
    private var mPlaylistItemAdapter: PlaylistItemAdapter? = null
    private lateinit var mMainLayout: BottomPanelLayout

    private lateinit var controllerFuture: ListenableFuture<MediaController>
    private val controller: MediaController?
        get() = if (controllerFuture.isDone) controllerFuture.get() else null

    override fun onOrientationChange(newConfig: Int) {
        super.onOrientationChange(newConfig)
        if (newConfig == Configuration.ORIENTATION_LANDSCAPE) {
            updateLandscapeView()
        } else if (newConfig == Configuration.ORIENTATION_PORTRAIT) {
            updatePortraitView()
        }
    }

    override fun initializeViews() {
        setContentView(R.layout.activity_playlist_player)

        mPlaylistToolbar = findViewById(R.id.playlistToolbar)
        mTvVideoTitle = findViewById(R.id.tvVideoTitle)
        mTvVideoSource = findViewById(R.id.tvVideoSource)
        mTvPlaylistName = findViewById(R.id.tvPlaylistName)
        mAspectRatioFrameLayout = findViewById(R.id.aspect_ratio_frame_layout)
        mPlayerView = findViewById(R.id.styledPlayerView)
        mHoverControlsLayout = findViewById(R.id.hover_controls_layout)
        mFullscreenImg = findViewById(R.id.fullscreen_img)
        mBackImg = findViewById(R.id.back_img)
        mEmptyView = findViewById(R.id.empty_view)
        mVideoSeekBar = findViewById(R.id.videoSeekBar)
        mTvVideoTimeElapsed = findViewById(R.id.tvVideoTimeElapsed)
        mTvVideoTimeRemaining = findViewById(R.id.tvVideoTimeRemaining)
        mIvPlaylistMediaSpeed = findViewById(R.id.ivPlaylistMediaSpeed)
        mIvPlaylistRepeat = findViewById(R.id.ivPlaylistRepeat)
        mIvPlaylistShuffle = findViewById(R.id.ivPlaylistShuffle)
        mIvNextVideo = findViewById(R.id.ivNextVideo)
        mIvPrevVideo = findViewById(R.id.ivPrevVideo)
        mIvPlayPauseVideo = findViewById(R.id.ivPlayPauseVideo)
        mIvSeekForward15Seconds = findViewById(R.id.ivSeekForward15Seconds)
        mIvSeekBack15Seconds = findViewById(R.id.ivSeekBack15Seconds)
        mIvVideoOptions = findViewById(R.id.ivVideoOptions)
        mVideoPlayerLoading = findViewById(R.id.videoPlayerLoading)
        mMainLayout = findViewById(R.id.sliding_layout)
        mLayoutBottom = findViewById(R.id.bottom_layout)
        mLayoutPlayer = findViewById(R.id.player_layout)
        mLayoutVideoControls = findViewById(R.id.layoutVideoControls)
        mRvPlaylist = findViewById(R.id.rvPlaylists)
        mProgressBar = findViewById(R.id.progressBar)

        mProgressBar.visibility = View.VISIBLE
        mRvPlaylist.visibility = View.GONE

        mAspectRatioFrameLayout.setAspectRatio(16f / 9f)
        // mPlayerView.setOnTouchListener { v, event ->
        //     when (event?.action) {
        //         MotionEvent.ACTION_DOWN -> {
        //             showHoveringControls()
        //         }
        //     }

        //     v?.onTouchEvent(event) ?: true
        // }
        mFullscreenImg.setOnClickListener {
            if (resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                // requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
            } else {
                requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
            }
            if (!isOrientationLocked()) {
                requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
            }
        }
        mBackImg.setOnClickListener {
            finish()
        }
        mIvVideoOptions.setOnClickListener {
            val currentPlaylistItem =
                    mPlaylist.items[mPlayerView.player?.currentPeriodIndex ?: 0]
                MenuUtils.showPlaylistItemMenu(
                    this@PlaylistPlayerActivity,
                    supportFragmentManager,
                    currentPlaylistItem,
                    playlistId = mPlaylist.id,
                    playlistItemOptionsListener = this@PlaylistPlayerActivity,
                    shouldShowMove = false
                )
        }
        mMainLayout.addPanelSlideListener(this)
        val mediaRouteButton: MediaRouteButton = findViewById(R.id.media_route_button)
        CastButtonFactory.setUpMediaRouteButton(this@PlaylistPlayerActivity, mediaRouteButton)

        mPlaylistItemAdapter = PlaylistItemAdapter(this@PlaylistPlayerActivity)
        mPlaylistItemAdapter?.setBottomLayout()
        mRvPlaylist.adapter = mPlaylistItemAdapter

        if (resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            updateLandscapeView()
        } else {
            updatePortraitView()
        }
    }

    override fun finishNativeInitialization() {
        super.finishNativeInitialization()
        
    }

    override fun onResumeWithNative() {
        Log.e(TAG, "onResumeWithNative")
        super.onResumeWithNative();
        mPlaylistId = intent.getStringExtra(ConstantUtils.PLAYLIST_ID)?:ConstantUtils.DEFAULT_PLAYLIST

        fetchPlaylistData()
    }

    private fun fetchPlaylistData() {
        mPlaylistService?.getPlaylist(mPlaylistId) {
                playlist -> 
                    Log.e(TAG, playlist.toString())
                    val playlistItems = mutableListOf<PlaylistItem>()
                    playlist.items.forEach {
                        if (it.id != controller?.currentMediaItem?.mediaId) {
                            playlistItems.add(it)
                        } else {
                            playlistItems.add(0, it)
                        }
                    }

                    val updatedPlaylist = Playlist()
                    updatedPlaylist.id = playlist.id
                    updatedPlaylist.name = playlist.name
                    updatedPlaylist.items = playlist.items

                    mPlaylist = updatedPlaylist

                    VideoPlaybackService.currentPlayingItem.observe(this@PlaylistPlayerActivity) { currentPlayingItemId ->
                        if (!currentPlayingItemId.isNullOrEmpty()) {
                            // mPlaylistItemAdapter?.updatePlayingStatus(currentPlayingItemId)
                        }
                    }

                    mTvVideoSource.text =
                        if (mPlaylist.id == DEFAULT_PLAYLIST) getString(R.string.playlist_play_later) else mPlaylist.name
                    mTvPlaylistName.text = getString(R.string.playlist_up_next)

                    mPlaylistItemAdapter?.submitList(playlistItems)

                    mProgressBar.visibility = View.GONE
                    mRvPlaylist.visibility = View.VISIBLE
                };
    }

    override fun onDestroy() {
        Log.e(TAG, "onDestroy")
        mPlayerView.player = null
        // releaseController()
        
        super.onDestroy();
    }

    override fun onStartWithNative() {
        Log.e(TAG, "onStartWithNative")
        super.onStartWithNative()
        initializeController()
    }

    override fun onStopWithNative() {
        releaseController()
        super.onStopWithNative()
    }


    private fun initializeController() {
        Log.e(TAG, "initializeController")
        controllerFuture = MediaController.Builder(
            this@PlaylistPlayerActivity, SessionToken(
                this@PlaylistPlayerActivity, ComponentName(this@PlaylistPlayerActivity, VideoPlaybackService::class.java)
            )
        ).buildAsync()
        controllerFuture.addListener({ setController() }, MoreExecutors.directExecutor())
    }

    private fun releaseController() {
        Log.e(TAG, "releaseController")
        MediaController.releaseFuture(controllerFuture)
    }

    private fun setController() {
        if (controller == null) {
            return
        }
        Log.e(TAG, "setController")
        updatePlayerView()

        initializePlayer()
    }

    private fun isOrientationLocked(): Boolean {
        val contentResolver: ContentResolver? =
            contentResolver // You can get this from your context
        return try {
            val rotationSetting: Int =
                Settings.System.getInt(contentResolver, Settings.System.ACCELEROMETER_ROTATION)
            rotationSetting == 0 // If rotationSetting is 0, it means the orientation is locked; if it's 1, it's unlocked.
        } catch (e: Settings.SettingNotFoundException) {
            e.printStackTrace()
            false // Unable to determine, return a default value
        }
    }

    private fun updatePlayerView() {
        setNextMedia()
        setPrevMedia()
        setSeekForward()
        setSeekBack()
        setSeekBarListener()
        setPlaylistShuffle()
        setPlaylistRepeatMode()
        setPlaybackSpeed()
    }

    private fun showHoveringControls() {
        val newVisibility = if (mHoverControlsLayout.isVisible) View.GONE else View.VISIBLE
        mHoverControlsLayout.visibility = newVisibility
        Looper.myLooper()?.let {
            Handler(it).postDelayed({
                if (mHoverControlsLayout.isVisible) mHoverControlsLayout.visibility = View.GONE
            }, 5000)
        }
    }

    override fun onMediaItemTransition(mediaItem: MediaItem?, reason: Int) {
        super.onMediaItemTransition(mediaItem, reason)
        Log.e(TAG, "onMediaItemTransition")
        mPlayerView.player?.let {
            updateSeekBar()
            mDuration = it.duration
            updateTime(it.currentPosition)
            mTvVideoTitle.text = mPlaylist.items[it.currentPeriodIndex].name
            mIvNextVideo.isEnabled = it.hasNextMediaItem()
            mIvNextVideo.alpha = if (it.hasNextMediaItem()) 1.0f else 0.4f
            mIvPrevVideo.isEnabled = it.hasPreviousMediaItem()
            mIvPrevVideo.alpha = if (it.hasPreviousMediaItem()) 1.0f else 0.4f
        }
        mVideoPlayerLoading.visibility = View.GONE
    }

    override fun onPlaybackStateChanged(playbackState: Int) {
        Log.e(TAG, "onPlaybackStateChanged")
        super.onPlaybackStateChanged(playbackState)
        if (playbackState == ExoPlayer.STATE_READY) {
            mPlayerView.player?.let {
                mDuration = it.duration
                updateTime(it.currentPosition)
                mTvVideoTitle.text = it.currentMediaItem?.mediaMetadata?.artist
                mIvNextVideo.isEnabled = it.hasNextMediaItem()
                mIvNextVideo.alpha = if (it.hasNextMediaItem()) 1.0f else 0.4f
                mIvPrevVideo.isEnabled = it.hasPreviousMediaItem()
                mIvPrevVideo.alpha = if (it.hasPreviousMediaItem()) 1.0f else 0.4f
            }
            mVideoPlayerLoading.visibility = View.GONE
        }
    }

    override fun onIsPlayingChanged(isPlaying: Boolean) {
        super.onIsPlayingChanged(isPlaying)
        Log.e(TAG, "onIsPlayingChanged")
        if (isPlaying && !mIsUserTrackingTouch) mPlayerView.postDelayed(
            this::setCurrentPlayerPosition, mUpdatePositionDelayMs
        )
        setPlayAndPause()
    }

    private fun setPlaybackSpeed() {
        mIvPlaylistMediaSpeed.setOnClickListener {
            mPlaybackSpeed += 0.5f
            if (mPlaybackSpeed > 2) mPlaybackSpeed = 1f
            when (mPlaybackSpeed) {
                1f -> mIvPlaylistMediaSpeed.setImageResource(R.drawable.ic_playlist_speed_1x)
                1.5f -> mIvPlaylistMediaSpeed.setImageResource(R.drawable.ic_playlist_speed_1_point_5_x)
                2f -> mIvPlaylistMediaSpeed.setImageResource(R.drawable.ic_playlist_speed_2x)
            }
            mUpdatePositionDelayMs = (mUpdatePositionDelayMs / mPlaybackSpeed).toLong()
            mPlayerView.player?.setPlaybackSpeed(mPlaybackSpeed)
        }
    }

    private fun setPlaylistRepeatMode() {
        mIvPlaylistRepeat.setOnClickListener {
            when (mRepeatMode) {
                Player.REPEAT_MODE_OFF -> {
                    mRepeatMode = Player.REPEAT_MODE_ALL
                    mIvPlaylistRepeat.setImageResource(R.drawable.ic_playlist_repeat_all_on)
                }

                Player.REPEAT_MODE_ALL -> {
                    mRepeatMode = Player.REPEAT_MODE_ONE
                    mIvPlaylistRepeat.setImageResource(R.drawable.ic_playlist_repeat_1)
                }

                Player.REPEAT_MODE_ONE -> {
                    mRepeatMode = Player.REPEAT_MODE_OFF
                    mIvPlaylistRepeat.setImageResource(R.drawable.ic_playlist_repeat_all_off)
                }
            }
            mPlayerView.player?.repeatMode = mRepeatMode
        }
    }

    private fun setPlaylistShuffle() {
        mIvPlaylistShuffle.setOnClickListener {
            mIsShuffleOn = !mIsShuffleOn
            mPlayerView.player?.shuffleModeEnabled = mIsShuffleOn
            mIvPlaylistShuffle.setImageResource(
                if (mIsShuffleOn) R.drawable.ic_playlist_shuffle_on
                else R.drawable.ic_playlist_shuffle_off
            )
        }
    }

    private fun initializePlayer() {
        Log.e(TAG, "initializePlayer")
        mPlayerView.player = controller
        controller?.addListener(this@PlaylistPlayerActivity)
        mPlayerView.player?.let { currentPlayer ->
            Log.e(TAG, "addListener")
            currentPlayer.shuffleModeEnabled = mIsShuffleOn
            currentPlayer.repeatMode = mRepeatMode
            currentPlayer.setPlaybackSpeed(mPlaybackSpeed)
        }
    }

    private fun releasePlayer() {
        mPlayerView.player?.let {
            it.removeListener(this@PlaylistPlayerActivity)
            mPlaybackPosition = it.currentPosition
            mCurrentMediaIndex = it.currentMediaItemIndex
            mPlayWhenReady = it.playWhenReady
            mIsShuffleOn = it.shuffleModeEnabled
        }
    }

    private fun disableNextPreviousControls() {
        mIvNextVideo.isEnabled = false
        mIvNextVideo.alpha = 0.4f
        mIvPrevVideo.isEnabled = false
        mIvPrevVideo.alpha = 0.4f
    }


    private fun setNextMedia() {
        mIvNextVideo.setOnClickListener {
            mPlayerView.player?.let {
                if (it.hasNextMediaItem()) {
                    it.seekToNextMediaItem()
                    disableNextPreviousControls()
                }
            }
        }
    }

    private fun setPrevMedia() {
        mIvPrevVideo.setOnClickListener {
            mPlayerView.player?.let {
                if (it.hasPreviousMediaItem()) {
                    it.seekToPreviousMediaItem()
                    disableNextPreviousControls()
                }
            }
        }
    }

    private fun setPlayAndPause() {
        setPlayOrPauseIcon(mIvPlayPauseVideo)
        mIvPlayPauseVideo.setOnClickListener {
            mPlayerView.player?.let {
                setPlayOrPauseIcon(mIvPlayPauseVideo)
                if (it.isPlaying) it.pause()
                else it.play()
            }
        }
    }

    private fun setPlayOrPauseIcon(ivPlayPauseVideo: AppCompatImageView) {
        mPlayerView.player?.let {
            ivPlayPauseVideo.setImageResource(
                if (!it.isPlaying) R.drawable.ic_playlist_pause_media
                else R.drawable.ic_playlist_play_media
            )
        }
    }

    private fun setSeekForward() {
        mIvSeekForward15Seconds.setOnClickListener {
            mPlayerView.player?.let {
                it.seekTo(it.currentPosition + SEEK_VALUE_MS)
                updateTime(it.currentPosition)
                updateSeekBar()
            }
        }
    }

    private fun setSeekBack() {
        mIvSeekBack15Seconds.setOnClickListener {
            mPlayerView.player?.let {
                it.seekTo(it.currentPosition - SEEK_VALUE_MS)
                updateTime(it.currentPosition)
                updateSeekBar()
            }
        }
    }

    private fun setSeekBarListener() {
        mVideoSeekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(
                seekBar: SeekBar?, progress: Int, fromUser: Boolean
            ) {
                mPlayerView.player?.let {
                    updateTime(it.currentPosition)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                mIsUserTrackingTouch = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                mIsUserTrackingTouch = false
                mPlayerView.player?.let {
                    val percentage = seekBar.progress.toFloat() / 100f
                    it.seekTo((mDuration * percentage).toLong())
                    updateTime(it.currentPosition)
                }
            }
        })
    }

    private fun setCurrentPlayerPosition() {
        mPlayerView.player?.let {
            if (it.isPlaying) {
                updateTime(it.currentPosition)
                mPlayerView.postDelayed(
                    this::setCurrentPlayerPosition, mUpdatePositionDelayMs
                )
            }
            if (it.isPlaying && !mIsUserTrackingTouch) updateSeekBar()
        }
    }

    private fun updateSeekBar() {
        mPlayerView.player?.let {
            mVideoSeekBar.progress =
                ((it.currentPosition.toFloat() / mDuration.toFloat()) * 100).toInt()
        }
    }

    private fun updateTime(currentPosition: Long) {
        mTvVideoTimeElapsed.text = getFormattedTime(currentPosition, false)
        mTvVideoTimeRemaining.text = getFormattedTime(mDuration - currentPosition, true)
    }

    private fun getFormattedTime(time: Long, isNegative: Boolean): String {
        val totalSeconds = time / 1000
        val seconds = totalSeconds % 60
        val minutes = (totalSeconds / 60) % 60
        val hours = totalSeconds / 3600
        val outSeconds = if (seconds < 10) "0$seconds" else "$seconds"
        val outMinutes = if (minutes < 10) "0${minutes}" else "$minutes"
        val outHours = if (hours == 0L) "" else if (hours < 10) "0$hours:" else "$hours:"
        return "${(if (isNegative) "-" else "")}$outHours$outMinutes:$outSeconds"
    }

    private fun enableControls(enable: Boolean, vg: ViewGroup) {
        for (i in 0 until vg.childCount) {
            val child = vg.getChildAt(i)
            child.isEnabled = enable
            if (child is ViewGroup) {
                enableControls(enable, child)
            }
        }
    }

    private fun updatePortraitView() {
        // view?.afterMeasured {
        //     view?.fitsSystemWindows = true
        // }

        val layoutParams: FrameLayout.LayoutParams =
            mAspectRatioFrameLayout.layoutParams as FrameLayout.LayoutParams
        mPlayerView.afterMeasured {
            layoutParams.height = TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                if (context?.resources?.getBoolean(R.bool.isTablet) == true) 650f else 300f,
                resources.displayMetrics
            ).toInt()
            layoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT
            mAspectRatioFrameLayout.layoutParams = layoutParams
            mAspectRatioFrameLayout.resizeMode = AspectRatioFrameLayout.RESIZE_MODE_FIT
        }

        val hoverLayoutParams: FrameLayout.LayoutParams =
            mHoverControlsLayout.layoutParams as FrameLayout.LayoutParams
        layoutParams.width = FrameLayout.LayoutParams.WRAP_CONTENT
        mHoverControlsLayout.layoutParams = hoverLayoutParams
        mHoverControlsLayout.setBackgroundResource(R.drawable.rounded_bg_16)

        mMainLayout.mSlideState = BottomPanelLayout.PanelState.COLLAPSED
        mTvVideoTitle.afterMeasured {
            val availableHeight = Integer.min(mMainLayout.measuredHeight.minus(mLayoutPlayer.bottom), 190.dpToPx.toInt())
            mMainLayout.panelHeight = availableHeight
        }

        if (!mIsCastInProgress) {
            mPlayerView.useController = false
            mLayoutVideoControls.visibility = View.VISIBLE
        }
        mPlaylistToolbar.visibility = View.VISIBLE
        val windowInsetsController = WindowCompat.getInsetsController(
            window, window.decorView
        )
        windowInsetsController.show(WindowInsetsCompat.Type.systemBars())
        mBackImg.visibility = View.GONE
        mEmptyView.visibility = View.GONE
        mFullscreenImg.setImageResource(R.drawable.ic_fullscreen)
    }

    private fun updateLandscapeView() {
        // view?.afterMeasured {
        //     view?.fitsSystemWindows = false
        // }
        val layoutParams: FrameLayout.LayoutParams =
            mAspectRatioFrameLayout.layoutParams as FrameLayout.LayoutParams
        mPlayerView.afterMeasured {
            layoutParams.width = FrameLayout.LayoutParams.MATCH_PARENT
            layoutParams.height = FrameLayout.LayoutParams.MATCH_PARENT
            mAspectRatioFrameLayout.layoutParams = layoutParams
            mAspectRatioFrameLayout.resizeMode = AspectRatioFrameLayout.RESIZE_MODE_FILL
        }

        val hoverLayoutParams: FrameLayout.LayoutParams =
            mHoverControlsLayout.layoutParams as FrameLayout.LayoutParams
        layoutParams.width = FrameLayout.LayoutParams.MATCH_PARENT
        mHoverControlsLayout.layoutParams = hoverLayoutParams
        mHoverControlsLayout.setBackgroundResource(R.color.player_control_bg)

        mMainLayout.mSlideState = BottomPanelLayout.PanelState.HIDDEN
        mPlayerView.useController = true
        mPlayerView.controllerHideOnTouch = true
        mPlayerView.showController()
        mLayoutVideoControls.visibility = View.GONE
        mPlaylistToolbar.visibility = View.GONE
        val windowInsetsController = WindowCompat.getInsetsController(
            window, window.decorView
        )
        windowInsetsController.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        windowInsetsController.hide(WindowInsetsCompat.Type.systemBars())
        mBackImg.visibility = View.VISIBLE
        mEmptyView.visibility = View.VISIBLE
        mFullscreenImg.setImageResource(R.drawable.ic_close_fullscreen)
    }

    override fun onPanelStateChanged(
        panel: View?,
        previousState: BottomPanelLayout.PanelState?,
        newState: BottomPanelLayout.PanelState?
    ) {
        val shouldEnableControls = newState != BottomPanelLayout.PanelState.EXPANDED
        enableControls(shouldEnableControls, mLayoutPlayer)
    }

    override fun onPlaylistItemClick(position: Int) {
        if (!PlaylistUtils.isPlaylistItemCached(mPlaylist.items[position])) {
            return
        }
        mPlayerView.player?.seekTo(position, 0)
        mMainLayout.smoothToBottom()
    }

    override fun onPlaylistItemMenuClick(view: View, playlistItem: PlaylistItem) {
        MenuUtils.showPlaylistItemMenu(
            this@PlaylistPlayerActivity,
            supportFragmentManager,
            playlistItem = playlistItem,
            playlistId = mPlaylistId,
            playlistItemOptionsListener = this,
            shouldShowMove = false
        )
    }

    override fun deletePlaylistItem(playlistItemOptionModel: PlaylistItemOptionModel) {
        playlistItemOptionModel.playlistItem?.let {
            mPlaylistService?.removeItemFromPlaylist(mPlaylistId, it.id)
            if (it.id == controller?.currentMediaItem?.mediaId) {
                mPlayerView.player?.stop()
                finish()
            } else {
                fetchPlaylistData()
            }
        }
    }

    override fun onMediaFileDownloadProgressed(id:String, totalBytes:Long, receivedBytes:Long, percentComplete:Byte, timeRemaining:String) {
        mPlaylistItemAdapter?.updatePlaylistItemDownloadProgress(HlsContentProgressModel(id, totalBytes, receivedBytes, "" + percentComplete))
    }

    override fun onItemCached(playlistItem:PlaylistItem) {
        Log.e(TAG, "onItemCached")
        mPlaylistItemAdapter?.updatePlaylistItem(playlistItem)
    }

    override fun onItemUpdated(playlistItem:PlaylistItem) {
        Log.e(TAG, "onItemUpdated")
        mPlaylistItemAdapter?.updatePlaylistItem(playlistItem)
    }

    override fun onPlaylistUpdated(playlist:Playlist) {
        Log.e(TAG, "onPlaylistUpdated")
        fetchPlaylistData()
    }

    override fun onEvent(eventType: Int , id : String) {
        if (eventType == PlaylistEvent.ITEM_MOVED && id.equals(mPlaylistId)) {
            fetchPlaylistData()
        }
    }

    // override fun onPlaylistItemOptionClicked(playlistItemOptionModel: PlaylistItemOptionModel) {
    //     if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM) {
    //         playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
    //             PlaylistUtils.showSharingDialog(
    //                 this@PlaylistPlayerActivity, it
    //             )
    //         }
    //     } else {
    //         if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM) {
    //             mPlayerView.player?.stop()
    //             // if (activity is AppCompatActivity) (activity as AppCompatActivity).onBackPressedDispatcher.onBackPressed()
    //             finish()
    //         } else if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM || playlistItemOptionModel.optionType == PlaylistOptionsEnum.COPY_PLAYLIST_ITEM) {
    //             val moveOrCopyItems = ArrayList<PlaylistItem>()
    //             playlistItemOptionModel.playlistItem?.let { moveOrCopyItems.add(it) }
    //             PlaylistUtils.moveOrCopyModel =
    //                 MoveOrCopyModel(playlistItemOptionModel.optionType,mPlaylistId, "", moveOrCopyItems)
    //         }
    //         // mPlaylistViewModel.setPlaylistItemOption(playlistItemOptionModel)
    //     }
    // }
}

/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.fragment

import android.annotation.SuppressLint
import android.content.*
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.net.Uri
import android.os.*
import android.util.Log
import android.util.TypedValue
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.ProgressBar
import android.widget.SeekBar
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
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
import androidx.mediarouter.app.MediaRouteButton
import androidx.recyclerview.widget.RecyclerView
import org.chromium.chrome.browser.playlist.kotlin.PlaylistVideoService
import com.brave.playlist.PlaylistViewModel
import org.chromium.chrome.R
import com.brave.playlist.adapter.recyclerview.PlaylistItemAdapter
import com.brave.playlist.enums.PlaylistOptionsEnum
import com.brave.playlist.extension.afterMeasured
import com.brave.playlist.extension.dpToPx
import com.brave.playlist.listener.PlaylistItemClickListener
import com.brave.playlist.listener.PlaylistItemOptionsListener
import com.brave.playlist.local_database.PlaylistRepository
import com.brave.playlist.model.*
import androidx.lifecycle.ViewModelStoreOwner
import com.brave.playlist.slidingpanel.BottomPanelLayout
import com.brave.playlist.util.ConnectionUtils
import com.brave.playlist.util.ConstantUtils.DEFAULT_PLAYLIST
import com.brave.playlist.util.ConstantUtils.PLAYER_ITEMS
import com.brave.playlist.util.ConstantUtils.PLAYLIST_MODEL
import com.brave.playlist.util.ConstantUtils.PLAYLIST_NAME
import com.brave.playlist.util.ConstantUtils.SELECTED_PLAYLIST_ITEM_ID
import com.brave.playlist.util.ConstantUtils.TAG
import com.brave.playlist.util.MediaUtils
import com.brave.playlist.util.MenuUtils
import com.brave.playlist.util.PlaylistPreferenceUtils
import com.brave.playlist.util.PlaylistPreferenceUtils.rememberFilePlaybackPosition
import com.brave.playlist.util.PlaylistUtils
import com.brave.playlist.view.PlaylistToolbar
import com.google.android.exoplayer2.ExoPlayer
import com.google.android.exoplayer2.MediaItem
import com.google.android.exoplayer2.Player
import com.google.android.exoplayer2.ui.AspectRatioFrameLayout
import com.google.android.exoplayer2.ui.StyledPlayerView
import com.google.android.gms.cast.framework.CastButtonFactory
import com.google.android.material.card.MaterialCardView
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import java.io.IOException

class PlaylistPlayerFragment : Fragment(R.layout.fragment_playlist_player), Player.Listener,
    PlaylistItemClickListener, PlaylistItemOptionsListener, BottomPanelLayout.PanelSlideListener {
    private val mScope = CoroutineScope(Job() + Dispatchers.IO)
    private lateinit var mPlaylistViewModel: PlaylistViewModel
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
    private lateinit var mStyledPlayerView: StyledPlayerView
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

    private var mPlaylistModel: PlaylistModel? = null
    private var mSelectedPlaylistItemId: String = ""
    private var mPlaylistItems = mutableListOf<PlaylistItemModel>()

    private lateinit var mRvPlaylist: RecyclerView
    private var mPlaylistItemAdapter: PlaylistItemAdapter? = null
    private lateinit var mMainLayout: BottomPanelLayout

    private var mPlaylistVideoService: PlaylistVideoService? = null

    private val mPlaylistRepository: PlaylistRepository by lazy {
        PlaylistRepository(requireContext())
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            updateLandscapeView()
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
            updatePortraitView()
        }
    }

    private fun updatePortraitView() {
        view?.afterMeasured {
            view?.fitsSystemWindows = true
        }

        val layoutParams: FrameLayout.LayoutParams =
            mAspectRatioFrameLayout.layoutParams as FrameLayout.LayoutParams
        mStyledPlayerView.afterMeasured {
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

        mMainLayout.mSlideState = BottomPanelLayout.PanelState.COLLAPSED
        mMainLayout.panelHeight =
            if (context?.resources?.getBoolean(R.bool.isTablet) == true) 150.dpToPx.toInt() else 70.dpToPx.toInt()
        if (!mIsCastInProgress) {
            mStyledPlayerView.useController = false
            mLayoutVideoControls.visibility = View.VISIBLE
        }
        mPlaylistToolbar.visibility = View.VISIBLE
        val windowInsetsController =
            WindowCompat.getInsetsController(
                requireActivity().window,
                requireActivity().window.decorView
            )
        windowInsetsController.show(WindowInsetsCompat.Type.systemBars())
        mBackImg.visibility = View.GONE
        mEmptyView.visibility = View.GONE
        mFullscreenImg.setImageResource(R.drawable.ic_fullscreen)
    }

    private fun updateLandscapeView() {
        view?.afterMeasured {
            view?.fitsSystemWindows = false
        }
        val layoutParams: FrameLayout.LayoutParams =
            mAspectRatioFrameLayout.layoutParams as FrameLayout.LayoutParams
        mStyledPlayerView.afterMeasured {
            layoutParams.width = FrameLayout.LayoutParams.MATCH_PARENT
            layoutParams.height = FrameLayout.LayoutParams.MATCH_PARENT
            mAspectRatioFrameLayout.layoutParams = layoutParams
            mAspectRatioFrameLayout.resizeMode = AspectRatioFrameLayout.RESIZE_MODE_FILL
        }

        val hoverLayoutParams: FrameLayout.LayoutParams =
            mHoverControlsLayout.layoutParams as FrameLayout.LayoutParams
        layoutParams.width = FrameLayout.LayoutParams.MATCH_PARENT
        mHoverControlsLayout.layoutParams = hoverLayoutParams

        mMainLayout.mSlideState = BottomPanelLayout.PanelState.HIDDEN
        mStyledPlayerView.useController = true
        mStyledPlayerView.controllerHideOnTouch = true
        mStyledPlayerView.showController()
        mLayoutVideoControls.visibility = View.GONE
        mPlaylistToolbar.visibility = View.GONE
        val windowInsetsController =
            WindowCompat.getInsetsController(
                requireActivity().window,
                requireActivity().window.decorView
            )
        windowInsetsController.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        windowInsetsController.hide(WindowInsetsCompat.Type.systemBars())
        mBackImg.visibility = View.VISIBLE
        mEmptyView.visibility = View.VISIBLE
        mFullscreenImg.setImageResource(R.drawable.ic_close_fullscreen)
    }

    private val connection = object : ServiceConnection {
        override fun onServiceDisconnected(name: ComponentName?) {
        }

        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            if (service is PlaylistVideoService.PlaylistVideoServiceBinder) {
                mPlaylistVideoService = service.getServiceInstance()

                setToolbar()
                setNextMedia()
                setPrevMedia()
                setSeekForward()
                setSeekBack()
                setSeekBarListener()
                setPlaylistShuffle()
                setPlaylistRepeatMode()
                setPlaybackSpeed()

                initializePlayer()
            }
        }
    }

    @Suppress("DEPRECATION")
    private fun getPlaylistModel(bundle: Bundle): PlaylistModel? {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            bundle.getParcelable(PLAYLIST_MODEL, PlaylistModel::class.java)
        } else {
            bundle.getParcelable(PLAYLIST_MODEL)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let {
            mPlaylistModel = getPlaylistModel(it)
            mSelectedPlaylistItemId = it.getString(SELECTED_PLAYLIST_ITEM_ID).toString()
            Log.e("data_source", "PlaylistPlayerFragment onCreate : "+mSelectedPlaylistItemId);
        }
    }

    @SuppressLint("SourceLockedOrientationActivity")
    override fun onDestroy() {
        activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        super.onDestroy()
    }

    @SuppressLint("ClickableViewAccessibility", "SourceLockedOrientationActivity")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        Log.e("data_source", "PlaylistPlayerFragment onViewCreated");
        mPlaylistViewModel = ViewModelProvider(requireActivity() as ViewModelStoreOwner)[PlaylistViewModel::class.java]

        activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED

        mPlaylistModel?.id?.let { mPlaylistViewModel.fetchPlaylistData(it) }

        mPlaylistToolbar = view.findViewById(R.id.playlistToolbar)
        mTvVideoTitle = view.findViewById(R.id.tvVideoTitle)
        mTvVideoSource = view.findViewById(R.id.tvVideoSource)
        mTvPlaylistName = view.findViewById(R.id.tvPlaylistName)

        mTvVideoSource.text =
            if (mPlaylistModel?.id == DEFAULT_PLAYLIST) getString(R.string.playlist_play_later) else mPlaylistModel?.name
        mTvPlaylistName.text =
            if (mPlaylistModel?.id == DEFAULT_PLAYLIST) getString(R.string.playlist_play_later) else mPlaylistModel?.name

        mAspectRatioFrameLayout = view.findViewById(R.id.aspect_ratio_frame_layout)
        mAspectRatioFrameLayout.setAspectRatio(16f / 9f)

        mStyledPlayerView = view.findViewById(R.id.styledPlayerView)
        mStyledPlayerView.setOnTouchListener { v, event ->
            when (event?.action) {
                MotionEvent.ACTION_DOWN -> {
                    showHoveringControls()
                }
            }

            v?.onTouchEvent(event) ?: true
        }
        mHoverControlsLayout = view.findViewById(R.id.hover_controls_layout)
        mFullscreenImg = view.findViewById(R.id.fullscreen_img)
        mFullscreenImg.setOnClickListener {
            if (resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
            } else {
                activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
            }
            activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        }

        mBackImg = view.findViewById(R.id.back_img)
        mBackImg.setOnClickListener {
        }

        mEmptyView = view.findViewById(R.id.empty_view)

        mVideoSeekBar = view.findViewById(R.id.videoSeekBar)
        mTvVideoTimeElapsed = view.findViewById(R.id.tvVideoTimeElapsed)
        mTvVideoTimeRemaining = view.findViewById(R.id.tvVideoTimeRemaining)
        mIvPlaylistMediaSpeed = view.findViewById(R.id.ivPlaylistMediaSpeed)
        mIvPlaylistRepeat = view.findViewById(R.id.ivPlaylistRepeat)
        mIvPlaylistShuffle = view.findViewById(R.id.ivPlaylistShuffle)
        mIvNextVideo = view.findViewById(R.id.ivNextVideo)
        mIvPrevVideo = view.findViewById(R.id.ivPrevVideo)
        mIvPlayPauseVideo = view.findViewById(R.id.ivPlayPauseVideo)
        mIvSeekForward15Seconds = view.findViewById(R.id.ivSeekForward15Seconds)
        mIvSeekBack15Seconds = view.findViewById(R.id.ivSeekBack15Seconds)
        mIvVideoOptions = view.findViewById(R.id.ivVideoOptions)
        mIvVideoOptions.setOnClickListener {
            mPlaylistModel?.let { model ->
                val currentPlaylistItem =
                    mPlaylistItems[mPlaylistVideoService?.getCurrentPlayer()?.currentPeriodIndex
                        ?: 0]
                MenuUtils.showPlaylistItemMenu(
                    view.context, parentFragmentManager,
                    currentPlaylistItem, playlistId = model.id, playlistItemOptionsListener = this,
                    shouldHideDeleteOption = true
                )
            }
        }
        mVideoPlayerLoading = view.findViewById(R.id.videoPlayerLoading)

        mMainLayout = view.findViewById(R.id.sliding_layout)
        mMainLayout.addPanelSlideListener(this)
        mLayoutBottom = view.findViewById(R.id.bottom_layout)
        mLayoutPlayer = view.findViewById(R.id.player_layout)
        mLayoutVideoControls = view.findViewById(R.id.layoutVideoControls)

        mRvPlaylist = view.findViewById(R.id.rvPlaylists)
        mProgressBar = view.findViewById(R.id.progressBar)
        mProgressBar.visibility = View.VISIBLE
        mRvPlaylist.visibility = View.GONE

        val mediaRouteButton: MediaRouteButton =
            view.findViewById(R.id.media_route_button)
        CastButtonFactory.setUpMediaRouteButton(requireContext(), mediaRouteButton)

        mPlaylistViewModel.playlistData.observe(viewLifecycleOwner) { playlistData ->
            mPlaylistItems = mutableListOf()
            playlistData.items.forEach {
                if (it.id != mSelectedPlaylistItemId) {
                    mPlaylistItems.add(it)
                } else {
                    mPlaylistItems.add(0, it)
                }
            }

            mPlaylistModel = PlaylistModel(
                playlistData.id,
                playlistData.name,
                mPlaylistItems
            )

            val intent = Intent(requireContext(), PlaylistVideoService::class.java)
                .apply {
                    putExtra(
                        PLAYLIST_NAME,
                        if (mPlaylistModel?.id == DEFAULT_PLAYLIST) getString(R.string.playlist_play_later) else mPlaylistModel?.name
                    )
                    putParcelableArrayListExtra(PLAYER_ITEMS, ArrayList(mPlaylistItems))
                }

            // Playlist Video service
            activity?.startService(intent)
            activity?.bindService(intent, connection, Context.BIND_AUTO_CREATE)

            PlaylistVideoService.castStatus.observe(viewLifecycleOwner) { shouldShowControls ->
                mIsCastInProgress = !shouldShowControls
                mLayoutVideoControls.visibility =
                    if (shouldShowControls) View.VISIBLE else View.GONE
            }

            PlaylistVideoService.currentPlayingItem.observe(viewLifecycleOwner) { currentPlayingItemId ->
                if (!currentPlayingItemId.isNullOrEmpty()) {
                    mPlaylistItemAdapter?.updatePlayingStatus(currentPlayingItemId)
                }
            }

            mScope.launch {
                mPlaylistItems.forEach {
                    try {
                        if (it.isCached) {
                            val fileSize =
                                MediaUtils.getFileSizeFromUri(view.context, Uri.parse(it.mediaPath))
                            it.fileSize = fileSize
                        }
                    } catch (ex: IOException) {
                        Log.e(TAG, ex.message.toString())
                    }
                }

                activity?.runOnUiThread {
                    // Bottom Layout set up
                    mPlaylistItemAdapter =
                        PlaylistItemAdapter(mPlaylistItems, this@PlaylistPlayerFragment)
                    mPlaylistItemAdapter?.setBottomLayout()
                    mRvPlaylist.adapter = mPlaylistItemAdapter

                    mProgressBar.visibility = View.GONE
                    mRvPlaylist.visibility = View.VISIBLE
                    mPlaylistViewModel.downloadProgress.observe(viewLifecycleOwner) {
                        mPlaylistItemAdapter?.updatePlaylistItemDownloadProgress(it)
                    }

                    mPlaylistViewModel.playlistItemEventUpdate.observe(viewLifecycleOwner) {
                        mPlaylistItemAdapter?.updatePlaylistItem(it)
                    }
                }
            }
        }

        if (resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            updateLandscapeView()
        } else {
            updatePortraitView()
        }
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

    override fun onDestroyView() {
        releasePlayer()
        mMainLayout.removePanelSlideListener(this)
        super.onDestroyView()
    }

    override fun onMediaItemTransition(mediaItem: MediaItem?, reason: Int) {
        super.onMediaItemTransition(mediaItem, reason)
        mPlaylistVideoService?.getCurrentPlayer()?.let {
            updateSeekBar()
            mDuration = it.duration
            updateTime(it.currentPosition)
            mTvVideoTitle.text = mPlaylistItems[it.currentPeriodIndex].name
            mIvNextVideo.isEnabled = it.hasNextMediaItem()
            mIvNextVideo.alpha = if (it.hasNextMediaItem()) 1.0f else 0.4f
            mIvPrevVideo.isEnabled = it.hasPreviousMediaItem()
            mIvPrevVideo.alpha = if (it.hasPreviousMediaItem()) 1.0f else 0.4f
        }
        mVideoPlayerLoading.visibility = View.GONE
    }

    override fun onPlaybackStateChanged(playbackState: Int) {
        super.onPlaybackStateChanged(playbackState)
        if (playbackState == ExoPlayer.STATE_READY) {
            mPlaylistVideoService?.getCurrentPlayer()?.let {
                mDuration = it.duration
                updateTime(it.currentPosition)
                mTvVideoTitle.text = mPlaylistItems[it.currentPeriodIndex].name
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
        if (isPlaying && !mIsUserTrackingTouch)
            mStyledPlayerView.postDelayed(this::setCurrentPlayerPosition, mUpdatePositionDelayMs)
        setPlayAndPause()
    }

    private fun setToolbar() {
        mPlaylistToolbar.setOptionsButtonClickListener {
        }
    }

    private fun setPlaybackSpeed() {
        mIvPlaylistMediaSpeed.setOnClickListener {
            mPlaybackSpeed += 0.5f
            if (mPlaybackSpeed > 2)
                mPlaybackSpeed = 1f
            when (mPlaybackSpeed) {
                1f -> mIvPlaylistMediaSpeed.setImageResource(R.drawable.ic_playlist_speed_1x)
                1.5f -> mIvPlaylistMediaSpeed.setImageResource(R.drawable.ic_playlist_speed_1_point_5_x)
                2f -> mIvPlaylistMediaSpeed.setImageResource(R.drawable.ic_playlist_speed_2x)
            }
            mUpdatePositionDelayMs = (mUpdatePositionDelayMs / mPlaybackSpeed).toLong()
            mPlaylistVideoService?.getCurrentPlayer()?.setPlaybackSpeed(mPlaybackSpeed)
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
            mPlaylistVideoService?.getCurrentPlayer()?.repeatMode = mRepeatMode
        }
    }

    private fun setPlaylistShuffle() {
        mIvPlaylistShuffle.setOnClickListener {
            mIsShuffleOn = !mIsShuffleOn
            mPlaylistVideoService?.getCurrentPlayer()?.shuffleModeEnabled = mIsShuffleOn
            mIvPlaylistShuffle.setImageResource(
                if (mIsShuffleOn)
                    R.drawable.ic_playlist_shuffle_on
                else
                    R.drawable.ic_playlist_shuffle_off
            )
        }
    }

    private fun initializePlayer() {
        mPlaylistVideoService?.setPlayerView(mStyledPlayerView)
        mStyledPlayerView.player = mPlaylistVideoService?.getCurrentPlayer()
        mPlaylistVideoService?.getCurrentPlayer()?.let { currentPlayer ->
            currentPlayer.addListener(this)
            currentPlayer.shuffleModeEnabled = mIsShuffleOn
            val currentPlaylistItemId = mPlaylistItems[currentPlayer.currentMediaItemIndex].id
            mScope.launch {
                mPlaylistRepository.getPlaylistItemById(currentPlaylistItemId)?.lastPlayedPosition?.let { lastPosition ->
                    val handler = Handler(currentPlayer.applicationLooper)
                    val runnableCode = Runnable {
                        currentPlayer.seekTo(
                            mCurrentMediaIndex,
                            if (PlaylistPreferenceUtils.defaultPrefs(requireContext()).rememberFilePlaybackPosition) lastPosition else 0
                        )
                    }
                    handler.post(runnableCode)
                }
            }
            currentPlayer.repeatMode = mRepeatMode
            currentPlayer.setPlaybackSpeed(mPlaybackSpeed)
        }
    }

    private fun releasePlayer() {
        mPlaylistVideoService?.getCurrentPlayer()?.let {
            it.removeListener(this)
            mPlaybackPosition = it.currentPosition
            mCurrentMediaIndex = it.currentMediaItemIndex
            mPlayWhenReady = it.playWhenReady
            mIsShuffleOn = it.shuffleModeEnabled
        }
        activity?.unbindService(connection)
    }

    private fun disableNextPreviousControls() {
        mIvNextVideo.isEnabled = false
        mIvNextVideo.alpha = 0.4f
        mIvPrevVideo.isEnabled = false
        mIvPrevVideo.alpha = 0.4f
    }


    private fun setNextMedia() {
        mIvNextVideo.setOnClickListener {
            mPlaylistVideoService?.getCurrentPlayer()?.let {
                if (it.hasNextMediaItem()) {
                    if (!mPlaylistItems[it.nextMediaItemIndex].isCached && !ConnectionUtils.isDeviceOnline(
                            requireContext()
                        )
                    ) {
                        Toast.makeText(
                            requireContext(),
                            getString(R.string.playlist_offline_message),
                            Toast.LENGTH_SHORT
                        ).show()
                    } else {
                        it.seekToNextMediaItem()
                        disableNextPreviousControls()
                    }
                }
            }
        }
    }

    private fun setPrevMedia() {
        mIvPrevVideo.setOnClickListener {
            mPlaylistVideoService?.getCurrentPlayer()?.let {
                if (it.hasPreviousMediaItem()) {
                    if (!mPlaylistItems[it.previousMediaItemIndex].isCached && !ConnectionUtils.isDeviceOnline(
                            requireContext()
                        )
                    ) {
                        Toast.makeText(
                            requireContext(),
                            getString(R.string.playlist_offline_message),
                            Toast.LENGTH_SHORT
                        ).show()
                    } else {
                        it.seekToPreviousMediaItem()
                        disableNextPreviousControls()
                    }
                }
            }
        }
    }

    private fun setPlayAndPause() {
        setPlayOrPauseIcon(mIvPlayPauseVideo)
        mIvPlayPauseVideo.setOnClickListener {
            mPlaylistVideoService?.getCurrentPlayer()?.let {
                setPlayOrPauseIcon(mIvPlayPauseVideo)
                if (it.isPlaying)
                    it.pause()
                else
                    it.play()
            }
        }
    }

    private fun setPlayOrPauseIcon(ivPlayPauseVideo: AppCompatImageView) {
        mPlaylistVideoService?.getCurrentPlayer()?.let {
            ivPlayPauseVideo.setImageResource(
                if (!it.isPlaying)
                    R.drawable.ic_playlist_pause_media
                else
                    R.drawable.ic_playlist_play_media
            )
        }
    }

    private fun setSeekForward() {
        mIvSeekForward15Seconds.setOnClickListener {
            mPlaylistVideoService?.getCurrentPlayer()?.let {
                it.seekTo(it.currentPosition + SEEK_VALUE_MS)
                updateTime(it.currentPosition)
                updateSeekBar()
            }
        }
    }

    private fun setSeekBack() {
        mIvSeekBack15Seconds.setOnClickListener {
            mPlaylistVideoService?.getCurrentPlayer()?.let {
                it.seekTo(it.currentPosition - SEEK_VALUE_MS)
                updateTime(it.currentPosition)
                updateSeekBar()
            }
        }
    }

    private fun setSeekBarListener() {
        mVideoSeekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(
                seekBar: SeekBar?,
                progress: Int,
                fromUser: Boolean
            ) {
                mPlaylistVideoService?.getCurrentPlayer()?.let {
                    updateTime(it.currentPosition)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                mIsUserTrackingTouch = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                mIsUserTrackingTouch = false
                mPlaylistVideoService?.getCurrentPlayer()?.let {
                    val percentage = seekBar.progress.toFloat() / 100f
                    it.seekTo((mDuration * percentage).toLong())
                    updateTime(it.currentPosition)
                }
            }
        })
    }

    private fun setCurrentPlayerPosition() {
        mPlaylistVideoService?.getCurrentPlayer()?.let {
            if (it.isPlaying) {
                updateTime(it.currentPosition)
                mStyledPlayerView.postDelayed(
                    this::setCurrentPlayerPosition,
                    mUpdatePositionDelayMs
                )
            }
            if (it.isPlaying && !mIsUserTrackingTouch)
                updateSeekBar()
        }
    }

    private fun updateSeekBar() {
        mPlaylistVideoService?.getCurrentPlayer()?.let {
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

    override fun onPlaylistItemClick(count: Int) {
        if (!mPlaylistItems[count].isCached && !ConnectionUtils.isDeviceOnline(requireContext())) {
            Toast.makeText(
                requireContext(),
                getString(R.string.playlist_offline_message),
                Toast.LENGTH_SHORT
            ).show()
            return
        }
        mPlaylistVideoService?.setCurrentItem(count)
        mMainLayout.smoothToBottom()
    }

    override fun onPlaylistItemMenuClick(view: View, playlistItemModel: PlaylistItemModel) {
        MenuUtils.showPlaylistItemMenu(
            view.context,
            parentFragmentManager,
            playlistItemModel = playlistItemModel,
            playlistId = playlistItemModel.playlistId,
            playlistItemOptionsListener = this,
            shouldHideDeleteOption = true
        )
    }

    override fun onOptionClicked(playlistItemOptionModel: PlaylistItemOptionModel) {
        if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM) {
            playlistItemOptionModel.playlistItemModel?.pageSource?.let {
                PlaylistUtils.showSharingDialog(
                    requireContext(),
                    it
                )
            }
        } else {
            if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM) {
                mPlaylistVideoService?.getCurrentPlayer()?.stop()
            } else if (playlistItemOptionModel.optionType == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM || playlistItemOptionModel.optionType == PlaylistOptionsEnum.COPY_PLAYLIST_ITEM) {
                val moveOrCopyItems = ArrayList<PlaylistItemModel>()
                playlistItemOptionModel.playlistItemModel?.let { moveOrCopyItems.add(it) }
                PlaylistUtils.moveOrCopyModel =
                    MoveOrCopyModel(playlistItemOptionModel.optionType, "", moveOrCopyItems)
            }
            mPlaylistViewModel.setPlaylistItemOption(playlistItemOptionModel)
        }
    }

    companion object {
        private const val SEEK_VALUE_MS = 15000

        @JvmStatic
        fun newInstance(selectedPlaylistItemId: String, playlistModel: PlaylistModel) =
            PlaylistPlayerFragment().apply {
            Log.e("data_source", "PlaylistPlayerFragment.newInstance");
                arguments = Bundle().apply {
                    putParcelable(PLAYLIST_MODEL, playlistModel)
                    putString(SELECTED_PLAYLIST_ITEM_ID, selectedPlaylistItemId)
                }
            }
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

    override fun onPanelStateChanged(
        panel: View?,
        previousState: BottomPanelLayout.PanelState?,
        newState: BottomPanelLayout.PanelState?
    ) {
        val shouldEnableControls = newState != BottomPanelLayout.PanelState.EXPANDED
        enableControls(shouldEnableControls, mLayoutPlayer)
    }
}

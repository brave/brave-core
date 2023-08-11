/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin

import org.chromium.chrome.R
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import org.chromium.chrome.browser.playlist.kotlin.PlaylistDownloadUtils
import com.google.android.exoplayer2.C
import com.google.android.exoplayer2.DefaultLoadControl
import com.google.android.exoplayer2.ExoPlayer
import android.os.Build
import com.google.android.exoplayer2.MediaItem
import com.google.android.exoplayer2.audio.AudioAttributes
import com.google.android.exoplayer2.source.DefaultMediaSourceFactory
import com.google.android.exoplayer2.ui.StyledPlayerView
import com.google.android.exoplayer2.util.MimeTypes
import org.chromium.chrome.browser.init.AsyncInitializationActivity
import com.google.android.exoplayer2.Player
import com.google.android.exoplayer2.PlaybackException
import android.util.Log
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker
import org.chromium.base.task.PostTask
import org.chromium.base.task.TaskTraits
import org.chromium.playlist.mojom.PlaylistService
import org.chromium.mojo.bindings.ConnectionErrorHandler
import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import org.chromium.mojo.system.MojoException
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl
import java.io.ByteArrayOutputStream
import java.io.File;
import java.io.FileOutputStream;
import android.os.Environment;
import java.nio.file.Files;
import java.nio.file.Paths;
import android.net.Uri
import java.nio.file.StandardOpenOption;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate

class TestVideoActivity : AsyncInitializationActivity(), Player.Listener, PlaylistStreamingObserverImplDelegate, ConnectionErrorHandler {
    private lateinit var mStyledPlayerView: StyledPlayerView
    private lateinit var mLocalPlayer: ExoPlayer
    private var mPlaylistService: PlaylistService? = null
    private var mPlaylistStreamingObserver:PlaylistStreamingObserverImpl? = null
    private var pipedOutputStream: PipedOutputStream? = null;
    private val file: File = File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                    "index.mp4");
    private var outputStream:FileOutputStream? = null;

    override public fun onConnectionError(e : MojoException) {
        mPlaylistService = null
        initPlaylistService()
    }

    fun initPlaylistService() {
        if (mPlaylistService != null) {
            return
        }

        mPlaylistService = PlaylistServiceFactoryAndroid.getInstance().getPlaylistService(this@TestVideoActivity)
    }

    override protected fun triggerLayoutInflation() {
        setContentView(R.layout.activity_test_video)
        onInitialLayoutInflationComplete()
    }

    override public fun finishNativeInitialization() {
        super.finishNativeInitialization()
        initPlaylistService()
        mPlaylistStreamingObserver = PlaylistStreamingObserverImpl(this@TestVideoActivity);
        mPlaylistService?.addObserverForStreaming(mPlaylistStreamingObserver);
        // if (mPlaylistService != null) {
            mPlaylistService?.queryPrompt("https://storage.googleapis.com/gtv-videos-bucket/sample/TearsOfSteel.mp4", "GET")
        // }
        preparePlayer();
    }

    override public fun shouldStartGpuProcess() : Boolean {
        return true
    }

    override fun onPlaybackStateChanged(playbackState: @Player.State Int) {
    	Log.e("data_source", "onPlaybackStateChanged : "+ playbackState.toString())
        if (playbackState == Player.STATE_READY) {
        	mLocalPlayer.play()
        }
    }

    override fun onPlayerError(error: PlaybackException) {
        super.onPlayerError(error)
        Log.e("data_source", "onPlayerError : " + error.message.toString())
    }

    override fun onMediaItemTransition(mediaItem: MediaItem?, reason: Int) {
        super.onMediaItemTransition(mediaItem, reason)
        
    }

    override fun onPositionDiscontinuity(
        oldPosition: Player.PositionInfo,
        newPosition: Player.PositionInfo,
        reason: @Player.DiscontinuityReason Int
    ) {
        
    }

    override fun onResponseStarted(url:String, contentLength:Long) {
        Log.e("data_source",
                "TestVideoActivity : onResponseStarted : " + url
                        + " Content length : " + contentLength)
        // preparePlayer();
        // try {
        //             pipedInputStream = PipedInputStream();
        // pipedOutputStream = PipedOutputStream(pipedInputStream);
        //         } catch (e:IOException) {
        //             e.printStackTrace();
        //         }
        BraveVpnNativeWorker.contentLength.complete(contentLength);
        // outputStream = FileOutputStream(file, true);
        // if (BraveVpnNativeWorker.output != null) {
        //     try {
        //         BraveVpnNativeWorker.output.reset()
        //     } catch (e:Exception) {
        //     }
        // } else {
        //     BraveVpnNativeWorker.output = ByteArrayOutputStream()
        // }
    }

    override fun onDataReceived(response: ByteArray) {
        Log.e("data_source", "TestVideoActivity : OnDataReceived : " + response.size)
        PostTask.postTask(TaskTraits.USER_VISIBLE_MAY_BLOCK,
            object : Runnable {
            override fun run() {
                // try {
                //     BraveVpnNativeWorker.output.write(response)
                //     // if (BraveVpnNativeWorker.output.toByteArray().size > BraveVpnNativeWorker.readLength) {
                //     //     BraveVpnNativeWorker.responseLength.complete(BraveVpnNativeWorker.output.toByteArray().size)
                //     // } else {

                //     // }
                // } catch (e:Exception) {
                //     Log.e("data_source", e.message.toString())
                // }
                // try {
                //     pipedOutputStream?.write(response);
                //     Log.e("data_source", "pipedInputStream length : "+pipedInputStream?.available());
                // } catch (e:IOException) {
                //     e.printStackTrace();
                // }
                // try {
                    // outputStream?.write(response);
                    // preparePlayer();
                    writeByteArrayToFile(response)
                    // Append data to the file
                    // if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    // Files.write(Paths.get(file.absolutePath), response, StandardOpenOption.APPEND);
                    // }

                    // var fileOutputStream:FileOutputStream = FileOutputStream(file.absolutePath, true);

                    // Write data to the file
                    // fileOutputStream?.write(response);

                    // Close the stream
                    // fileOutputStream.close();
                // } catch (e:IOException) {
                //     e.printStackTrace();
                // }
            }
        })
    }

    override fun onDataCompleted() {
        Log.e("data_source", "TestVideoActivity : onDataCompleted : file.getAbsolutePath() : ")
        // outputStream?.close();
    }

    private fun preparePlayer() {
        val loadControl = DefaultLoadControl.Builder()
            .setBufferDurationsMs(32 * 1024, 64 * 1024, 1024, 1024)
            .build()
        val audioAttributes: AudioAttributes = AudioAttributes.Builder()
            .setUsage(C.USAGE_MEDIA)
            .setContentType(C.CONTENT_TYPE_MOVIE)
            .build()
        val mLocalPlayer = ExoPlayer.Builder(applicationContext)
            .setMediaSourceFactory(
                DefaultMediaSourceFactory(
                    PlaylistDownloadUtils.getDataSourceFactory(
                        applicationContext
                    )
                )
            )
            .setLoadControl(loadControl)
            .setReleaseTimeoutMs(5000).setAudioAttributes(audioAttributes, true).build()
        mLocalPlayer.videoScalingMode = C.VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING
        mStyledPlayerView = findViewById(R.id.styledPlayerView)
        mStyledPlayerView.player = mLocalPlayer
        mStyledPlayerView.useController = true
        val mediaItem = MediaItem.Builder()
            .setUri(Uri.fromFile(
                File(
                    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                    "index.mp4")
            ))
            .setMimeType(MimeTypes.VIDEO_MP4)
            .build()
        mLocalPlayer.addMediaItem(mediaItem)
        mLocalPlayer.prepare()
        mLocalPlayer.playWhenReady = true
    }

    @Synchronized
    fun writeByteArrayToFile(byteArray: ByteArray) {
        var outputStream : FileOutputStream? = null
        try {
            val file = File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                    "index.mp4")
            Log.e("data_source", "filepath : "+file.absolutePath);
            outputStream = FileOutputStream(file, true)
            outputStream.write(byteArray)
        } catch (e: IOException) {
            e.printStackTrace()
        } finally {
            outputStream?.close()
        }
    }

    companion object {
        private const val SEEK_VALUE_MS = 15000
        @JvmField
        public var pipedInputStream:PipedInputStream? = null;
    }
}

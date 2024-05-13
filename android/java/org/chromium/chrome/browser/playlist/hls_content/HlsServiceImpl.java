/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.hls_content;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.media3.exoplayer.hls.playlist.HlsMediaPlaylist.Segment;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.kotlin.model.HlsContentProgressModel;
import org.chromium.chrome.browser.playlist.kotlin.playback_service.VideoPlaybackService;
import org.chromium.chrome.browser.playlist.kotlin.util.MediaUtils;
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistService;

import java.util.Queue;

public class HlsServiceImpl extends HlsService.Impl implements ConnectionErrorHandler {
    private static final String TAG = "Playlist/HlsServiceImpl";
    private final IBinder mBinder = new LocalBinder();
    private Context mContext = ContextUtils.getApplicationContext();
    private PlaylistService mPlaylistService;
    public static String currentDownloadingPlaylistItemId = "";

    class LocalBinder extends Binder {
        HlsServiceImpl getService() {
            return HlsServiceImpl.this;
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        initPlaylistService();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startHlsContentFromQueue();
        return Service.START_NOT_STICKY;
    }

    private void startHlsContentFromQueue() {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    // PlaylistRepository playlistRepository = new PlaylistRepository(mContext);
                    // if (playlistRepository == null) {
                    //     return;
                    // }
                    // HlsContentQueueModel hlsContentQueueModel =
                    //         playlistRepository.getFirstHlsContentQueueModel();
                    // if (hlsContentQueueModel == null || mPlaylistService == null) {
                    //     return;
                    // }

                    if (mPlaylistService == null) {
                        return;
                    }

                    mPlaylistService.getFirstHlsContent(
                            hlsContent -> {
                                mPlaylistService.getPlaylistItem(
                                        hlsContent.playlistItemId,
                                        playlistItem -> {
                                            if (playlistItem == null) {
                                                PostTask.postTask(
                                                        TaskTraits.USER_VISIBLE_MAY_BLOCK,
                                                        () -> {
                                                            // removeContentAndStartNextDownload(hlsContent.playlistItemId);
                                                        });
                                            }
                                            currentDownloadingPlaylistItemId =
                                                    hlsContent.playlistItemId;
                                            HlsUtils.getManifestFile(
                                                    mContext,
                                                    mPlaylistService,
                                                    playlistItem,
                                                    new HlsUtils.HlsManifestDelegate() {
                                                        @Override
                                                        public void onHlsManifestCompleted(
                                                                Queue<Segment> segmentsQueue) {
                                                            int total = segmentsQueue.size();
                                                            String hlsMediaFilePath =
                                                                    HlsUtils.getHlsMediaFilePath(
                                                                            playlistItem);
                                                            HlsUtils.deleteFileIfExist(
                                                                    hlsMediaFilePath);
                                                            HlsUtils.getHLSFile(
                                                                    mContext,
                                                                    mPlaylistService,
                                                                    playlistItem,
                                                                    segmentsQueue,
                                                                    new HlsUtils.HlsFileDelegate() {
                                                                        @Override
                                                                        public void onProgress(
                                                                                int sofar) {
                                                                            Log.e(
                                                                                    TAG,
                                                                                    "onProgress : "
                                                                                            + sofar);
                                                                            if (total > 0) {
                                                                                PlaylistUtils
                                                                                        .updateHlsContentProgress(
                                                                                                new HlsContentProgressModel(
                                                                                                        playlistItem
                                                                                                                .id,
                                                                                                        (long)
                                                                                                                total,
                                                                                                        (long)
                                                                                                                sofar,
                                                                                                        String
                                                                                                                .valueOf(
                                                                                                                        (sofar
                                                                                                                                        * 100)
                                                                                                                                / total)));
                                                                            }
                                                                        }

                                                                        @Override
                                                                        public void onReady(
                                                                                String mediaPath) {
                                                                            Log.e(
                                                                                    TAG,
                                                                                    "onReady : "
                                                                                            + mediaPath);
                                                                            PostTask.postTask(
                                                                                    TaskTraits
                                                                                            .BEST_EFFORT_MAY_BLOCK,
                                                                                    () -> {
                                                                                        long
                                                                                                updatedFileSize =
                                                                                                        MediaUtils
                                                                                                                .getFileSizeFromUri(
                                                                                                                        mContext,
                                                                                                                        Uri
                                                                                                                                .parse(
                                                                                                                                        "file://"
                                                                                                                                                + mediaPath));
                                                                                        mPlaylistService
                                                                                                .updateItemHlsMediaFilePath(
                                                                                                        playlistItem
                                                                                                                .id,
                                                                                                        mediaPath,
                                                                                                        updatedFileSize);
                                                                                        addNewPlaylistItemModel(
                                                                                                playlistItem
                                                                                                        .id);
                                                                                        removeContentAndStartNextDownload(
                                                                                                playlistItem
                                                                                                        .id);
                                                                                    });
                                                                        }
                                                                    });
                                                        }
                                                    });
                                        });
                            });
                });
    }

    private void removeContentAndStartNextDownload(String playlistItemId) {
        // PlaylistRepository playlistRepository = new PlaylistRepository(mContext);
        // if (playlistRepository == null) {
        //     return;
        // }
        // playlistRepository.deleteHlsContentQueueModel(playlistItemId);

        mPlaylistService.removeHlsContent(playlistItemId);
        currentDownloadingPlaylistItemId = "";
        mPlaylistService.getFirstHlsContent(
                hlsContent -> {
                    if (hlsContent != null) {
                        startHlsContentFromQueue();
                    } else {
                        getService().stopSelf();
                    }
                });
    }

    private void addNewPlaylistItemModel(String playlistItemId) {
        mPlaylistService.getPlaylistItem(
                playlistItemId,
                playlistItem -> {
                    if (HlsUtils.isVideoPlaybackServiceRunning()) {
                        VideoPlaybackService.Companion.addNewPlaylistItemModel(playlistItem);
                    }
                });
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mPlaylistService != null) {
            mPlaylistService.close();
            mPlaylistService = null;
        }
        if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)) {
            mPlaylistService = null;
            initPlaylistService();
        }
    }

    private void initPlaylistService() {
        if (mPlaylistService != null) {
            return;
        }

        mPlaylistService =
                PlaylistServiceFactoryAndroid.getInstance()
                        .getPlaylistService(
                                ProfileManager.getLastUsedRegularProfile(), HlsServiceImpl.this);
    }

    @Override
    public void onDestroy() {
        if (mPlaylistService != null) {
            mPlaylistService.close();
            mPlaylistService = null;
        }
        super.onDestroy();
    }
}

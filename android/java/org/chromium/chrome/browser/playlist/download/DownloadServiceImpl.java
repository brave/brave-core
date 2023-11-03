/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.download;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.media3.exoplayer.hls.playlist.HlsMediaPlaylist.Segment;

import com.brave.playlist.enums.DownloadStatus;
import com.brave.playlist.local_database.PlaylistRepository;
import com.brave.playlist.model.DownloadProgressModel;
import com.brave.playlist.model.DownloadQueueModel;
import com.brave.playlist.model.PlaylistItemModel;
import com.brave.playlist.playback_service.VideoPlaybackService;
import com.brave.playlist.util.MediaUtils;
import com.brave.playlist.util.PlaylistUtils;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistService;

import java.util.Queue;

public class DownloadServiceImpl extends DownloadService.Impl implements ConnectionErrorHandler {
    private static final String TAG = "Playlist.DownloadServiceImpl";
    private final IBinder mBinder = new LocalBinder();
    private Context mContext = ContextUtils.getApplicationContext();
    private PlaylistService mPlaylistService;

    class LocalBinder extends Binder {
        DownloadServiceImpl getService() {
            return DownloadServiceImpl.this;
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
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            initPlaylistService();
        }
    }

    private void startDownloadFromQueue() {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    PlaylistRepository playlistRepository = new PlaylistRepository(mContext);
                    if (playlistRepository != null) {
                        DownloadQueueModel downloadQueueModel =
                                playlistRepository.getFirstDownloadQueueModel();
                        if (downloadQueueModel != null && mPlaylistService != null) {
                            String playlistItemId = downloadQueueModel.getPlaylistItemId();
                            mPlaylistService.getPlaylistItem(
                                    playlistItemId,
                                    playlistItem -> {
                                        DownloadUtils.downloadManifestFile(
                                                mContext,
                                                mPlaylistService,
                                                playlistItem,
                                                new DownloadUtils.HlsManifestDownloadDelegate() {
                                                    @Override
                                                    public void onHlsManifestDownloadCompleted(
                                                            Queue<Segment> segmentsQueue) {
                                                        int total = segmentsQueue.size();
                                                        String hlsMediaFilePath =
                                                                DownloadUtils.getHlsMediaFilePath(
                                                                        playlistItem);
                                                        DownloadUtils.deleteFileIfExist(
                                                                hlsMediaFilePath);
                                                        DownloadUtils.downalodHLSFile(
                                                                mContext,
                                                                mPlaylistService,
                                                                playlistItem,
                                                                segmentsQueue,
                                                                new DownloadUtils
                                                                        .HlsFileDownloadDelegate() {
                                                                    @Override
                                                                    public void onDownloadProgress(
                                                                            int downloadedSofar) {
                                                                        PlaylistUtils
                                                                                .updateDownloadProgress(
                                                                                        new DownloadProgressModel(
                                                                                                playlistItem
                                                                                                        .id,
                                                                                                (long)
                                                                                                        total,
                                                                                                (long)
                                                                                                        downloadedSofar,
                                                                                                String
                                                                                                        .valueOf(
                                                                                                                (downloadedSofar
                                                                                                                                * 100)
                                                                                                                        / total)));
                                                                    }

                                                                    @Override
                                                                    public void onDownloadCompleted(
                                                                            String mediaPath) {
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
                                                                                    playlistRepository
                                                                                            .updateDownloadQueueModel(
                                                                                                    new DownloadQueueModel(
                                                                                                            playlistItem
                                                                                                                    .id,
                                                                                                            DownloadStatus
                                                                                                                    .DOWNLOADED
                                                                                                                    .name()));
                                                                                    addNewPlaylistItemModel(
                                                                                            playlistItem
                                                                                                    .id);
                                                                                    if (playlistRepository
                                                                                                    .getFirstDownloadQueueModel()
                                                                                            != null) {
                                                                                        startDownloadFromQueue();
                                                                                    } else {
                                                                                        getService()
                                                                                                .stopSelf();
                                                                                    }
                                                                                });
                                                                    }
                                                                });
                                                    }
                                                });
                                    });
                        }
                    }
                });
    }

    private void addNewPlaylistItemModel(String playlistItemId) {
        mPlaylistService.getPlaylistItem(
                playlistItemId,
                playlistItem -> {
                    PlaylistItemModel playlistItemModel =
                            new PlaylistItemModel(
                                    playlistItem.id,
                                    "default",
                                    playlistItem.name,
                                    playlistItem.pageSource.url,
                                    playlistItem.mediaPath.url,
                                    playlistItem.hlsMediaPath.url,
                                    playlistItem.mediaSource.url,
                                    playlistItem.thumbnailPath.url,
                                    playlistItem.author,
                                    playlistItem.duration,
                                    playlistItem.lastPlayedPosition,
                                    playlistItem.mediaFileBytes,
                                    playlistItem.cached,
                                    false);
                    VideoPlaybackService.Companion.addNewPlaylistItemModel(playlistItemModel);
                });
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startDownloadFromQueue();
        return Service.START_NOT_STICKY;
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && SharedPreferencesManager.getInstance()
                        .readBoolean(BravePlaylistPreferences.PREF_ENABLE_PLAYLIST, true)) {
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
                        .getPlaylistService(DownloadServiceImpl.this);
    }
}

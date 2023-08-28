/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.download;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.brave.playlist.enums.DownloadStatus;
import com.brave.playlist.local_database.PlaylistRepository;
import com.brave.playlist.model.DownloadQueueModel;
import com.brave.playlist.model.PlaylistItemModel;
import com.brave.playlist.util.ConstantUtils;
import com.brave.playlist.util.HLSParsingUtil;
import com.brave.playlist.util.PlaylistUtils;
import com.google.android.exoplayer2.source.hls.playlist.HlsMediaPlaylist.Segment;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate;
import org.chromium.chrome.browser.playlist.download.CancelDownloadBroadcastReceiver;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;
import org.chromium.playlist.mojom.PlaylistStreamingObserver;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;

public class DownloadServiceImpl extends DownloadService.Impl implements ConnectionErrorHandler {
    private static final String TAG = "Playlist.DownloadServiceImpl";
    private final IBinder mBinder = new LocalBinder();
    private static final int BRAVE_PLAYLIST_DOWNLOAD_NOTIFICATION_ID = 901;
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

    private DownloadQueueModel getFirstPlaylistItemToDownload() {
        // DownloadQueueModel downloadQueueModel = null;
        PlaylistRepository playlistRepository = new PlaylistRepository(mContext);
        // Log.e("playlist", "getFirstPlaylistItemToDownload : 1");
        // if (playlistRepository.getAllDownloadQueueModel() != null
        //         && playlistRepository.getAllDownloadQueueModel().size() > 0) {
        //     Log.e("playlist", "getFirstPlaylistItemToDownload : 2");
        //     downloadQueueModel = playlistRepository.getAllDownloadQueueModel().get(0);
        // }
        // Log.e("playlist", "getFirstPlaylistItemToDownload : 3");
        // // playlistRepository.closeRepository();
        return playlistRepository.getFirstDownloadQueueModel();
    }

    private void deletePlaylistItemFromDownloadQueue(DownloadQueueModel downloadQueueModel) {
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
            PlaylistRepository playlistRepository = new PlaylistRepository(mContext);
            if (playlistRepository != null) {
                playlistRepository.deleteDownloadQueueModel(downloadQueueModel);
            }
            // playlistRepository.closeRepository();
        });
    }

    private void startDownloadFromQueue() {
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
            DownloadQueueModel downloadQueueModel = getFirstPlaylistItemToDownload();
            if (mPlaylistService != null && downloadQueueModel != null) {
                getService().startForeground(BRAVE_PLAYLIST_DOWNLOAD_NOTIFICATION_ID,
                        getDownloadNotification("", false, 0, 0));
                String playlistItemId = downloadQueueModel.getPlaylistItemId();
                mPlaylistService.getPlaylistItem(playlistItemId, playlistItem -> {
                    DownloadUtils.downloadManifestFile(mContext, mPlaylistService, playlistItem,
                            new DownloadUtils.HlsManifestDownloadDelegate() {
                                @Override
                                public void onHlsManifestDownloadCompleted(
                                        Queue<Segment> segmentsQueue) {
                                    int total = segmentsQueue.size();
                                    String hlsMediaFilePath =
                                            DownloadUtils.getHlsMediaFilePath(playlistItem);
                                    DownloadUtils.deleteFileIfExist(hlsMediaFilePath);
                                    DownloadUtils.downalodHLSFile(mContext, mPlaylistService,
                                            playlistItem, segmentsQueue,
                                            new DownloadUtils.HlsFileDownloadDelegate() {
                                                @Override
                                                public void onDownloadProgress(
                                                        int downloadedSofar) {
                                                    updateDownloadNotification(playlistItem.name,
                                                            true, total, downloadedSofar);
                                                }

                                                @Override
                                                public void onDownloadCompleted(String mediaPath) {
                                                    // new Handler(Looper.getMainLooper()).post(()
                                                    // -> {
                                                    PostTask.postTask(
                                                            TaskTraits.BEST_EFFORT_MAY_BLOCK,
                                                            () -> {
                                                                PlaylistRepository playlistRepository =
                                                                        new PlaylistRepository(
                                                                                mContext);
                                                                if (playlistRepository != null) {
                                                                    mPlaylistService
                                                                            .updateItemHlsMediaFilePath(
                                                                                    playlistItem.id,
                                                                                    mediaPath);
                                                                    playlistRepository.updateDownloadQueueModel(
                                                                            new DownloadQueueModel(
                                                                                    playlistItem.id,
                                                                                    DownloadStatus
                                                                                            .DOWNLOADING
                                                                                            .name()));
                                                                    getService().stopForeground(
                                                                            true);
                                                                    getService().stopSelf();
                                                                }
                                                                // playlistRepository.closeRepository();
                                                            });
                                                    // mPlaylistService.updateItemHlsMediaFilePath(
                                                    //         playlistItem.id, mediaPath);
                                                    // deletePlaylistItemFromDownloadQueue(
                                                    //         downloadQueueModel);
                                                    // });
                                                    // Log.e("playlist", "startDownloadFromQueue :
                                                    // 1"); DownloadQueueModel downloadQueue =
                                                    // getFirstPlaylistItemToDownload();
                                                    // Log.e("playlist", "startDownloadFromQueue :
                                                    // 2"+ downloadQueue); if (downloadQueue !=
                                                    // null) {
                                                    //     Log.e("playlist", "startDownloadFromQueue
                                                    //     : 3"); startDownloadFromQueue();
                                                    // } else {
                                                    // getService().stopForeground(true);
                                                    // getService().stopSelf();
                                                    // }
                                                }
                                            });
                                }
                            });
                });
            }
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
                && SharedPreferencesManager.getInstance().readBoolean(
                        BravePlaylistPreferences.PREF_ENABLE_PLAYLIST, true)) {
            mPlaylistService = null;
            initPlaylistService();
        }
    }

    private void initPlaylistService() {
        if (mPlaylistService != null) {
            return;
        }

        mPlaylistService = PlaylistServiceFactoryAndroid.getInstance().getPlaylistService(
                DownloadServiceImpl.this);
    }

    private Notification getDownloadNotification(
            String notificationText, boolean shouldShowProgress, int total, int downloadedSofar) {
        Intent cancelDownloadIntent = new Intent(mContext, CancelDownloadBroadcastReceiver.class);
        cancelDownloadIntent.setAction(CancelDownloadBroadcastReceiver.CANCEL_DOWNLOAD_ACTION);
        PendingIntent cancelDownloadPendingIntent =
                PendingIntent.getBroadcast(mContext, 0, cancelDownloadIntent,
                        PendingIntent.FLAG_UPDATE_CURRENT
                                | IntentUtils.getPendingIntentMutabilityFlag(true));

        NotificationCompat.Builder notificationBuilder =
                new NotificationCompat.Builder(mContext, BraveActivity.CHANNEL_ID);
        notificationBuilder.setSmallIcon(R.drawable.ic_vpn)
                .setAutoCancel(false)
                .setContentTitle("Playlist download")
                .setContentText(notificationText)
                .setStyle(new NotificationCompat.BigTextStyle().bigText(notificationText))
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .addAction(R.drawable.ic_add_media_to_playlist,
                        mContext.getResources().getString(R.string.cancel),
                        cancelDownloadPendingIntent)
                .setOnlyAlertOnce(true);

        if (shouldShowProgress) {
            notificationBuilder.setProgress(total, downloadedSofar, false);
        }

        return notificationBuilder.build();
    }

    private void updateDownloadNotification(
            String notificationText, boolean shouldShowProgress, int total, int downloadedSofar) {
        Notification notification = getDownloadNotification(
                notificationText, shouldShowProgress, total, downloadedSofar);
        NotificationManager mNotificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.notify(BRAVE_PLAYLIST_DOWNLOAD_NOTIFICATION_ID, notification);
    }

    @Override
    public void onDestroy() {
        Log.e("playlist", "DownloadServiceImpl onDestroy");
        Intent intent = new Intent(mContext, DownloadService.class);
        mContext.startService(intent);
        super.onDestroy();
    }
}

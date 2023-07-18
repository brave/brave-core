/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin

import android.app.Notification
import android.content.Context
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.PLAYLIST_CHANNEL_ID
import com.google.android.exoplayer2.offline.Download
import com.google.android.exoplayer2.offline.DownloadManager
import com.google.android.exoplayer2.offline.DownloadService
import com.google.android.exoplayer2.scheduler.PlatformScheduler
import com.google.android.exoplayer2.scheduler.Scheduler
import com.google.android.exoplayer2.ui.DownloadNotificationHelper
import com.google.android.exoplayer2.util.NotificationUtil
import com.google.android.exoplayer2.util.Util
import org.chromium.chrome.R

class PlaylistDownloadService : DownloadService(
    1,
    DEFAULT_FOREGROUND_NOTIFICATION_UPDATE_INTERVAL,
    PLAYLIST_CHANNEL_ID,
    R.string.playlist_feature_text,
    R.string.playlist_feature_text
) {
    override fun getDownloadManager(): DownloadManager {
        val downloadManager: DownloadManager? =
            PlaylistDownloadUtils.getDownloadManager( /* context= */applicationContext)
        val downloadNotificationHelper: DownloadNotificationHelper =
            PlaylistDownloadUtils.getDownloadNotificationHelper( /* context= */this)
        downloadManager?.maxParallelDownloads = 5
        downloadManager!!.addListener(
            TerminalStateNotificationHelper(
                this, downloadNotificationHelper, 1 + 1
            )
        )
        return downloadManager
    }

    override fun getScheduler(): Scheduler {
        return PlatformScheduler(this, 1)
    }

    override fun getForegroundNotification(
        downloads: MutableList<Download>,
        notMetRequirements: Int
    ): Notification {
        return PlaylistDownloadUtils.getDownloadNotificationHelper(/* context= */ this)
            .buildProgressNotification(
                /* context = */ this,
                R.drawable.ic_downloaded,
                /* contentIntent = */ null,
                /* message = */ null,
                downloads,
                notMetRequirements
            )
    }

    private class TerminalStateNotificationHelper(
        context: Context, notificationHelper: DownloadNotificationHelper, firstNotificationId: Int
    ) :
        DownloadManager.Listener {
        private val context: Context
        private val notificationHelper: DownloadNotificationHelper
        private var nextNotificationId: Int

        init {
            this.context = context.applicationContext
            this.notificationHelper = notificationHelper
            nextNotificationId = firstNotificationId
        }

        override fun onDownloadChanged(
            downloadManager: DownloadManager,
            download: Download,
            finalException: Exception?
        ) {
            val notification: Notification = when (download.state) {
                Download.STATE_COMPLETED -> {
                    notificationHelper.buildDownloadCompletedNotification(
                        context,
                        R.drawable.ic_downloaded,
                        /* contentIntent = */ null,
                        Util.fromUtf8Bytes(download.request.data)
                    )
                }

                Download.STATE_FAILED -> {
                    notificationHelper.buildDownloadFailedNotification(
                        context,
                        R.drawable.ic_downloaded,  /* contentIntent = */
                        null,
                        Util.fromUtf8Bytes(download.request.data)
                    )
                }

                else -> {
                    return
                }
            }
            NotificationUtil.setNotification(context, nextNotificationId++, notification)
        }
    }
}

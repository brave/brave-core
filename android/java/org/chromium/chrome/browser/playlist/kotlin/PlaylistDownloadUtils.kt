/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin

import android.content.Context
import android.net.Uri
import android.util.Log
import com.brave.playlist.model.PlaylistItemModel
import com.brave.playlist.util.ConstantUtils
import org.chromium.chrome.browser.playlist.BraveChromiumHttpDataSource
import com.google.android.exoplayer2.MediaItem
import com.google.android.exoplayer2.database.DatabaseProvider
import com.google.android.exoplayer2.database.StandaloneDatabaseProvider
import com.google.android.exoplayer2.offline.Download
import com.google.android.exoplayer2.offline.DownloadManager
import com.google.android.exoplayer2.offline.DownloadRequest
import com.google.android.exoplayer2.offline.DownloadService
import com.google.android.exoplayer2.ui.DownloadNotificationHelper
import com.google.android.exoplayer2.upstream.DataSource
import com.google.android.exoplayer2.upstream.DefaultDataSource
import com.google.android.exoplayer2.upstream.DefaultHttpDataSource
import com.google.android.exoplayer2.upstream.cache.Cache
import com.google.android.exoplayer2.upstream.cache.CacheDataSource
import com.google.android.exoplayer2.upstream.cache.NoOpCacheEvictor
import com.google.android.exoplayer2.upstream.cache.SimpleCache
import com.google.android.exoplayer2.util.MimeTypes
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker
import com.google.android.exoplayer2.upstream.ByteArrayDataSource
import java.io.File
import org.chromium.chrome.R
import java.util.concurrent.Executors

object PlaylistDownloadUtils {
    private var mDataSourceFactory: DataSource.Factory? = null
//    private var mHttpDataSourceFactory: DataSource.Factory? = null
    private var mDatabaseProvider: DatabaseProvider? = null
    private var mDownloadDirectory: File? = null
    private var mDownloadCache: Cache? = null
    private var mDownloadManager: DownloadManager? = null

//    @Synchronized
//    fun getHttpDataSourceFactory(): DataSource.Factory? {
////        if (mHttpDataSourceFactory == null) {
////            mHttpDataSourceFactory = BraveChromiumHttpDataSource.Factory()
////        }
//        return BraveChromiumHttpDataSource.Factory()
//    }

    @Synchronized
    fun getDataSourceFactory(context: Context): DataSource.Factory {
        if (mDataSourceFactory == null) {
            // val upstreamFactory = DataSource.Factory { BraveChromiumHttpDataSource(BraveVpnNativeWorker.output.toByteArray()) }
            val upstreamFactory = DataSource.Factory { BraveChromiumHttpDataSource() }
            mDataSourceFactory =
                getDownloadCache(context)?.let { buildReadOnlyCacheDataSource(upstreamFactory, it) }
        }
        return mDataSourceFactory!!
    }

    @Synchronized
    fun getDownloadNotificationHelper(
        context: Context
    ): DownloadNotificationHelper {
        return DownloadNotificationHelper(context, ConstantUtils.PLAYLIST_CHANNEL_ID)
    }

    @Synchronized
    fun getDownloadManager(context: Context): DownloadManager? {
        ensureDownloadManagerInitialized(context)
        return mDownloadManager
    }

    @Synchronized
    private fun getDownloadCache(context: Context): Cache? {
        if (mDownloadCache == null) {
            val downloadContentDirectory =
                File(getDownloadDirectory(context), ConstantUtils.DOWNLOAD_CONTENT_DIRECTORY)
            mDownloadCache = SimpleCache(
                downloadContentDirectory, NoOpCacheEvictor(), getDatabaseProvider(context)!!
            )
        }
        return mDownloadCache
    }

    @Synchronized
    private fun ensureDownloadManagerInitialized(context: Context) {
        if (mDownloadManager == null) {
            mDownloadManager = getDownloadCache(context)?.let {
                DownloadManager(
                    context,
                    getDatabaseProvider(context)!!,
                    it,
                    getDataSourceFactory(context),
                    Executors.newFixedThreadPool( /* nThreads = */6)
                )
            }
        }
    }

    @Synchronized
    private fun getDatabaseProvider(context: Context): DatabaseProvider? {
        if (mDatabaseProvider == null) {
            mDatabaseProvider = StandaloneDatabaseProvider(context)
        }
        return mDatabaseProvider
    }

    @Synchronized
    private fun getDownloadDirectory(context: Context): File? {
        if (mDownloadDirectory == null) {
            mDownloadDirectory = context.filesDir
        }
        return mDownloadDirectory
    }

    private fun buildReadOnlyCacheDataSource(
        upstreamFactory: DataSource.Factory, cache: Cache
    ): CacheDataSource.Factory {
        return CacheDataSource.Factory()
            .setCache(cache)
            .setUpstreamDataSourceFactory(upstreamFactory)
            .setCacheWriteDataSinkFactory(null)
            .setFlags(CacheDataSource.FLAG_IGNORE_CACHE_ON_ERROR)
    }

    @JvmStatic
    fun startDownloadRequest(context: Context, playlistIteModel: PlaylistItemModel) {
        val extension: String = playlistIteModel.mediaPath
            .substring(playlistIteModel.mediaPath.lastIndexOf("."))
        Log.e(ConstantUtils.TAG, "extension : $extension")
        if (playlistIteModel.isCached && extension == ".m3u8") {
            val downloadRequest =
                DownloadRequest.Builder(playlistIteModel.id, Uri.parse(playlistIteModel.mediaSrc))
                    .setMimeType(
                        MimeTypes.APPLICATION_M3U8
                    ).build()
            if (getDownloadManager(context)?.downloadIndex?.getDownload(playlistIteModel.id)?.state != Download.STATE_COMPLETED) {
                DownloadService.sendAddDownload(
                    context,
                    PlaylistDownloadService::class.java,
                    downloadRequest,
                    true
                )
            }
            Log.e(ConstantUtils.TAG, playlistIteModel.name + playlistIteModel.mediaSrc)
        }
    }

    @JvmStatic
    fun removeDownloadRequest(context: Context, playlistIteModel: PlaylistItemModel) {
        val extension: String = playlistIteModel.mediaPath
            .substring(playlistIteModel.mediaPath.lastIndexOf("."))
        Log.e(ConstantUtils.TAG, "extension : $extension")
        if (playlistIteModel.isCached && extension == ".m3u8") {
            DownloadService.sendRemoveDownload(
                context,
                PlaylistDownloadService::class.java,
                playlistIteModel.id,
                true
            )
            Log.e(ConstantUtils.TAG, playlistIteModel.name + playlistIteModel.mediaSrc)
        }
    }

    fun getMediaItemFromDownloadRequest(
        context: Context,
        playlistIteModel: PlaylistItemModel
    ): MediaItem? {
        val extension: String = playlistIteModel.mediaPath
            .substring(playlistIteModel.mediaPath.lastIndexOf("."))
        return if (playlistIteModel.isCached && extension == ".m3u8") {
            Log.e(ConstantUtils.TAG, "extension : $extension")
            Log.e(
                ConstantUtils.TAG,
                getDownloadManager(context)?.downloadIndex?.getDownload(playlistIteModel.id)?.state.toString()
            )
            val downloadRequest =
                DownloadRequest.Builder(playlistIteModel.id, Uri.parse(playlistIteModel.mediaSrc))
                    .setMimeType(MimeTypes.APPLICATION_M3U8).build()
            Log.e(ConstantUtils.TAG, playlistIteModel.name + playlistIteModel.mediaSrc)
            downloadRequest.toMediaItem()
        } else {
            null
        }
    }
}

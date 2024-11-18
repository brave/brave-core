/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.playlist.hls_content;

import android.content.Context;
import android.text.TextUtils;

import androidx.media3.common.util.UriUtil;
import androidx.media3.exoplayer.hls.playlist.HlsMediaPlaylist.Segment;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl;
import org.chromium.chrome.browser.playlist.kotlin.playback_service.VideoPlaybackService;
import org.chromium.chrome.browser.playlist.kotlin.util.HLSParsingUtil;
import org.chromium.chrome.browser.playlist.kotlin.util.MediaUtils;
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;

import java.io.File;
import java.util.Queue;

public class HlsUtils {
    private static final String GET_METHOD = "GET";
    private static final String TAG = "Playlist/HlsUtils";

    public interface HlsManifestDelegate {
        default void onHlsManifestCompleted(Queue<Segment> segmentsQueue) {}
    }

    public interface HlsFileDelegate {
        default void onProgress(int sofar) {}

        default void onReady(String filePath) {}
    }

    private static int sSofar;

    public static void getManifestFile(
            Context context,
            PlaylistService playlistService,
            PlaylistItem playlistItem,
            HlsUtils.HlsManifestDelegate hlsManifestDelegate) {
        if (playlistService == null) {
            return;
        }
        String hlsManifestFilePath = getHlsManifestFilePath(playlistItem);
        final String manifestUrl = getHlsResolutionManifestUrl(context, playlistItem);
        if (TextUtils.isEmpty(manifestUrl)) {
            return;
        }
        PlaylistStreamingObserverImpl playlistStreamingObserverImpl =
                new PlaylistStreamingObserverImpl();
        PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate
                playlistStreamingObserverImplDelegate =
                        new PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate() {
                            @Override
                            public void onResponseStarted(String url, long contentLength) {
                                try {
                                    deleteFileIfExist(hlsManifestFilePath);
                                } catch (Exception ex) {
                                    Log.e(TAG, ex.getMessage());
                                }
                            }

                            @Override
                            public void onDataReceived(byte[] response) {
                                try {
                                    MediaUtils.writeToFile(response, hlsManifestFilePath);
                                } catch (Exception ex) {
                                    Log.e(TAG, ex.getMessage());
                                }
                            }

                            @Override
                            public void onDataCompleted() {
                                try {
                                    if (playlistStreamingObserverImpl != null
                                            && playlistStreamingObserverImpl.getDelegate()
                                                    != null) {
                                        playlistStreamingObserverImpl.close();
                                        playlistStreamingObserverImpl.destroy();
                                    }
                                    Queue<Segment> segmentsQueue =
                                            HLSParsingUtil.getContentSegments(
                                                    hlsManifestFilePath, manifestUrl);
                                    hlsManifestDelegate.onHlsManifestCompleted(segmentsQueue);
                                    sSofar = 0;
                                } catch (Exception ex) {
                                    Log.e(TAG, ex.getMessage());
                                }
                            }
                        };
        playlistStreamingObserverImpl.setDelegate(playlistStreamingObserverImplDelegate);
        playlistService.requestStreamingQuery(
                playlistItem.id, manifestUrl, GET_METHOD, playlistStreamingObserverImpl);
    }

    public static void getHLSFile(
            Context context,
            PlaylistService playlistService,
            PlaylistItem playlistItem,
            Queue<Segment> segmentsQueue,
            HlsUtils.HlsFileDelegate hlsFileDelegate) {
        if (playlistService == null) {
            return;
        }
        Segment segment = segmentsQueue.poll();
        if (segment == null) {
            return;
        }
        String mediaPath = getHlsMediaFilePath(playlistItem);
        final String manifestUrl = getHlsResolutionManifestUrl(context, playlistItem);
        PlaylistStreamingObserverImpl playlistStreamingObserverImpl =
                new PlaylistStreamingObserverImpl();
        PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate
                playlistStreamingObserverImplDelegate =
                        new PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate() {
                            @Override
                            public void onResponseStarted(String url, long contentLength) {
                                // unused for manifest file
                            }

                            @Override
                            public void onDataReceived(byte[] response) {
                                try {
                                    MediaUtils.writeToFile(response, mediaPath);
                                } catch (Exception ex) {
                                    Log.e(TAG, ex.getMessage());
                                }
                            }

                            @Override
                            public void onDataCompleted() {
                                try {
                                    if (playlistStreamingObserverImpl != null
                                            && playlistStreamingObserverImpl.getDelegate()
                                                    != null) {
                                        playlistStreamingObserverImpl.close();
                                        playlistStreamingObserverImpl.destroy();
                                    }
                                    sSofar++;
                                    Segment newSegment = segmentsQueue.peek();
                                    if (newSegment != null) {
                                        if (hlsFileDelegate != null) {
                                            hlsFileDelegate.onProgress(sSofar);
                                        }
                                        getHLSFile(
                                                context,
                                                playlistService,
                                                playlistItem,
                                                segmentsQueue,
                                                hlsFileDelegate);
                                    } else {
                                        if (hlsFileDelegate != null) {
                                            hlsFileDelegate.onReady(mediaPath);
                                        }
                                    }
                                } catch (Exception ex) {
                                    Log.e(TAG, ex.getMessage());
                                }
                            }
                        };
        playlistStreamingObserverImpl.setDelegate(playlistStreamingObserverImplDelegate);
        playlistService.requestStreamingQuery(
                playlistItem.id,
                UriUtil.resolve(manifestUrl, segment.url),
                GET_METHOD,
                playlistStreamingObserverImpl);
    }

    private static String getPlaylistIdFromFile(PlaylistItem playlistItem) {
        String playlistId = "Default";
        if (playlistItem != null && playlistItem.cached) {
            // For i.e.
            // "file:///data/user/0/com.brave.browser_nightly/app_chrome/Default/playlist/399C40F34AF31E593D0C48B9ECEEB4CA/media_file.m3u8"
            String[] paths = playlistItem.mediaPath.url.split(File.separator);
            if (paths.length - 4 > 0) {
                String playlistItemIdFromPath = paths[paths.length - 2];
                String playlistIdFromPath = paths[paths.length - 4];
                if (playlistItemIdFromPath != null
                        && playlistIdFromPath != null
                        && playlistItem.id.equals(playlistItemIdFromPath)) {
                    playlistId = playlistIdFromPath;
                }
            }
        }
        return playlistId;
    }

    public static String getHlsMediaFilePath(PlaylistItem playlistItem) {
        return PathUtils.getDataDirectory()
                + File.separator
                + getPlaylistIdFromFile(playlistItem)
                + File.separator
                + "playlist"
                + File.separator
                + playlistItem.id
                + File.separator
                + "media_file.mp4";
    }

    public static String getHlsManifestFilePath(PlaylistItem playlistItem) {
        return PathUtils.getDataDirectory()
                + File.separator
                + getPlaylistIdFromFile(playlistItem)
                + File.separator
                + "playlist"
                + File.separator
                + playlistItem.id
                + File.separator
                + "index.m3u8";
    }

    public static String getHlsResolutionManifestUrl(Context context, PlaylistItem playlistItem) {
        return HLSParsingUtil.getContentManifestUrl(
                context, playlistItem.mediaSource.url, playlistItem.mediaPath.url);
    }

    public static void deleteFileIfExist(String filePath) {
        File mediaFile = new File(filePath);
        if (mediaFile.exists()) {
            mediaFile.delete();
        }
    }

    public static boolean isVideoPlaybackServiceRunning() {
        return PlaylistUtils.isServiceRunning(
                ContextUtils.getApplicationContext(), VideoPlaybackService.class);
    }
}

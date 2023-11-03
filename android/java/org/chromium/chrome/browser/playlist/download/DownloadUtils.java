/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.playlist.download;

import android.content.Context;

import androidx.media3.common.util.UriUtil;
import androidx.media3.exoplayer.hls.playlist.HlsMediaPlaylist.Segment;

import com.brave.playlist.util.HLSParsingUtil;
import com.brave.playlist.util.MediaUtils;

import org.chromium.base.PathUtils;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;

import java.io.File;
import java.util.Queue;

public class DownloadUtils {
    private static final String GET_METHOD = "GET";
    private static final String TAG = "Playlist.DownloadUtils";

    public interface HlsManifestDownloadDelegate {
        default void onHlsManifestDownloadCompleted(Queue<Segment> segmentsQueue) {}
    }

    public interface HlsFileDownloadDelegate {
        default void onDownloadProgress(int downloadedSofar) {}

        default void onDownloadCompleted(String filePath) {}
    }

    private static int sDownloadedSofar;
    private static PlaylistStreamingObserverImpl sPlaylistStreamingObserverImpl;

    public static void downloadManifestFile(
            Context context,
            PlaylistService playlistService,
            PlaylistItem playlistItem,
            DownloadUtils.HlsManifestDownloadDelegate hlsManifestDownloadDelegate) {
        String mediaPath = getHlsMediaFilePath(playlistItem);
        String hlsManifestFilePath = getHlsManifestFilePath(playlistItem);
        final String manifestUrl = getHlsResolutionManifestUrl(context, playlistItem);
        if (playlistService != null) {
            playlistService.requestStreamingQuery(manifestUrl, GET_METHOD);
            PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate
                    playlistStreamingObserverImplDelegate =
                            new PlaylistStreamingObserverImpl
                                    .PlaylistStreamingObserverImplDelegate() {
                                @Override
                                public void onResponseStarted(String url, long contentLength) {
                                    try {
                                        deleteFileIfExist(hlsManifestFilePath);
                                    } catch (Exception ex) {
                                        ex.printStackTrace();
                                    }
                                }

                                @Override
                                public void onDataReceived(byte[] response) {
                                    try {
                                        MediaUtils.writeToFile(response, hlsManifestFilePath);
                                    } catch (Exception ex) {
                                        ex.printStackTrace();
                                    }
                                }

                                @Override
                                public void onDataCompleted() {
                                    try {
                                        if (sPlaylistStreamingObserverImpl != null) {
                                            sPlaylistStreamingObserverImpl.close();
                                            sPlaylistStreamingObserverImpl.destroy();
                                        }
                                        playlistService.clearObserverForStreaming();
                                        Queue<Segment> segmentsQueue =
                                                HLSParsingUtil.getContentSegments(
                                                        hlsManifestFilePath, manifestUrl);
                                        hlsManifestDownloadDelegate.onHlsManifestDownloadCompleted(
                                                segmentsQueue);
                                        sDownloadedSofar = 0;
                                    } catch (Exception ex) {
                                        ex.printStackTrace();
                                    }
                                }
                            };
            sPlaylistStreamingObserverImpl =
                    new PlaylistStreamingObserverImpl(playlistStreamingObserverImplDelegate);
            playlistService.addObserverForStreaming(sPlaylistStreamingObserverImpl);
        }
    }

    public static void downalodHLSFile(
            Context context,
            PlaylistService playlistService,
            PlaylistItem playlistItem,
            Queue<Segment> segmentsQueue,
            DownloadUtils.HlsFileDownloadDelegate hlsFileDownloadDelegate) {
        if (playlistService != null) {
            Segment segment = segmentsQueue.poll();
            if (segment == null) {
                return;
            }
            String mediaPath = getHlsMediaFilePath(playlistItem);
            final String manifestUrl = getHlsResolutionManifestUrl(context, playlistItem);
            playlistService.requestStreamingQuery(
                    UriUtil.resolve(manifestUrl, segment.url), GET_METHOD);
            PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate
                    playlistStreamingObserverImplDelegate =
                            new PlaylistStreamingObserverImpl
                                    .PlaylistStreamingObserverImplDelegate() {
                                @Override
                                public void onResponseStarted(String url, long contentLength) {
                                    // unused for manifest file
                                }

                                @Override
                                public void onDataReceived(byte[] response) {
                                    try {
                                        MediaUtils.writeToFile(response, mediaPath);
                                    } catch (Exception ex) {
                                        ex.printStackTrace();
                                    }
                                }

                                @Override
                                public void onDataCompleted() {
                                    try {
                                        if (sPlaylistStreamingObserverImpl != null) {
                                            sPlaylistStreamingObserverImpl.close();
                                            sPlaylistStreamingObserverImpl.destroy();
                                        }
                                        playlistService.clearObserverForStreaming();
                                        sDownloadedSofar++;
                                        Segment newSegment = segmentsQueue.peek();
                                        if (newSegment != null) {
                                            if (hlsFileDownloadDelegate != null) {
                                                hlsFileDownloadDelegate.onDownloadProgress(
                                                        sDownloadedSofar);
                                            }
                                            downalodHLSFile(
                                                    context,
                                                    playlistService,
                                                    playlistItem,
                                                    segmentsQueue,
                                                    hlsFileDownloadDelegate);
                                        } else {
                                            if (hlsFileDownloadDelegate != null) {
                                                hlsFileDownloadDelegate.onDownloadCompleted(
                                                        mediaPath);
                                            }
                                        }
                                    } catch (Exception ex) {
                                        ex.printStackTrace();
                                    }
                                }
                            };
            sPlaylistStreamingObserverImpl =
                    new PlaylistStreamingObserverImpl(playlistStreamingObserverImplDelegate);
            playlistService.addObserverForStreaming(sPlaylistStreamingObserverImpl);
        }
    }

    private static String getPlaylistIdFromFile(PlaylistItem playlistItem) {
        String playlistId = "Default";
        if (playlistItem != null && playlistItem.cached) {
            String playlistItemMediaPath = playlistItem.mediaPath.url;
            String[] paths = playlistItem.mediaPath.url.split(File.separator);
            if (playlistItem.id.equals(paths[paths.length - 2])
                    && paths[paths.length - 4] != null) {
                playlistId = paths[paths.length - 4];
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
}

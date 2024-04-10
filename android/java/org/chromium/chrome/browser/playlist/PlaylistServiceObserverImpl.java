/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import org.chromium.chrome.browser.playlist.hls_content.HlsUtils;
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel;
import org.chromium.chrome.browser.playlist.kotlin.playback_service.VideoPlaybackService;
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils;
import org.chromium.chrome.browser.playlist.kotlin.util.MediaUtils;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.Playlist;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistServiceObserver;
import org.chromium.url.mojom.Url;

public class PlaylistServiceObserverImpl implements PlaylistServiceObserver {
    private static final String TAG = "Playlist/PlaylistServiceObserverImpl";

    public interface PlaylistServiceObserverImplDelegate {
        default void onItemCreated(PlaylistItem item) {}

        default void onItemLocalDataDeleted(String playlistItemId) {}

        default void onItemAddedToList(String playlistId, String playlistItemId) {}

        default void onItemRemovedFromList(String playlistId, String playlistItemId) {}

        default void onItemCached(PlaylistItem item) {}

        default void onItemUpdated(PlaylistItem item) {}

        default void onPlaylistUpdated(Playlist list) {}

        default void onEvent(int event, String playlistId) {}
        default void onMediaFileDownloadProgressed(String id, long totalBytes, long receivedBytes,
                byte percentComplete, String timeRemaining) {}
        default void onMediaFilesUpdated(Url pageUrl, PlaylistItem[] items) {}
    }

    private PlaylistServiceObserverImplDelegate mDelegate;

    public PlaylistServiceObserverImpl(PlaylistServiceObserverImplDelegate delegate) {
        mDelegate = delegate;
    }

    @Override
    public void onItemCreated(PlaylistItem item) {
        if (mDelegate == null) return;
        mDelegate.onItemCreated(item);
    }

    @Override
    public void onItemLocalDataDeleted(String playlistItemId) {
        if (mDelegate == null) return;
        mDelegate.onItemLocalDataDeleted(playlistItemId);
        if (HlsUtils.isVideoPlaybackServiceRunning()) {
            VideoPlaybackService.Companion.removePlaylistItemModel(playlistItemId);
        }
    }

    @Override
    public void onItemAddedToList(String playlistId, String playlistItemId) {
        if (mDelegate == null) return;
        mDelegate.onItemAddedToList(playlistId, playlistItemId);
    }

    @Override
    public void onItemRemovedFromList(String playlistId, String playlistItemId) {
        if (mDelegate == null) return;
        mDelegate.onItemRemovedFromList(playlistId, playlistItemId);
        if (HlsUtils.isVideoPlaybackServiceRunning()) {
            VideoPlaybackService.Companion.removePlaylistItemModel(playlistItemId);
        }
    }

    @Override
    public void onItemCached(PlaylistItem playlistItem) {
        if (mDelegate == null) return;
        mDelegate.onItemCached(playlistItem);

        if (!MediaUtils.isHlsFile(playlistItem.mediaPath.url)
                && HlsUtils.isVideoPlaybackServiceRunning()) {
            PlaylistItemModel playlistItemModel =
                    new PlaylistItemModel(
                            playlistItem.id,
                            ConstantUtils.DEFAULT_PLAYLIST,
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
        }
    }

    @Override
    public void onItemUpdated(PlaylistItem playlistItem) {
        if (mDelegate == null) return;
        mDelegate.onItemUpdated(playlistItem);
    }

    @Override
    public void onPlaylistUpdated(Playlist playlist) {
        if (mDelegate == null) return;
        mDelegate.onPlaylistUpdated(playlist);
    }

    @Override
    public void onEvent(int event, String playlistId) {
        if (mDelegate == null) return;
        mDelegate.onEvent(event, playlistId);
    }

    @Override
    public void onMediaFileDownloadScheduled(String id) {}

    @Override
    public void onMediaFileDownloadProgressed(
            String id,
            long totalBytes,
            long receivedBytes,
            byte percentComplete,
            String timeRemaining) {
        if (mDelegate == null) return;
        mDelegate.onMediaFileDownloadProgressed(
                id, totalBytes, receivedBytes, percentComplete, timeRemaining);
    }

    @Override
    public void onMediaFilesUpdated(Url pageUrl, PlaylistItem[] items) {
        if (mDelegate == null) return;
        mDelegate.onMediaFilesUpdated(pageUrl, items);
    }

    @Override
    public void close() {
        mDelegate = null;
    }

    @Override
    public void onConnectionError(MojoException e) {}

    public void destroy() {
        mDelegate = null;
    }
}

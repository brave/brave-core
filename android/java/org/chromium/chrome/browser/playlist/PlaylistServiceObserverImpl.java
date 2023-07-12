/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistServiceObserver;
import org.chromium.url.mojom.Url;

public class PlaylistServiceObserverImpl implements PlaylistServiceObserver {
    public interface PlaylistServiceObserverImplDelegate {
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
    public void onEvent(int event, String playlistId) {
        if (mDelegate == null) return;

        mDelegate.onEvent(event, playlistId);
    }

    @Override
    public void onMediaFileDownloadProgressed(String id, long totalBytes, long receivedBytes,
            byte percentComplete, String timeRemaining) {
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
    public void onItemCreated(PlaylistItem item) {}

    @Override
    public void onItemAddedToList(String playlistId, String itemId) {}

    @Override
    public void onItemRemovedFromList(String playlistId, String itemId) {}

    @Override
    public void onItemDeleted(String id) {}

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

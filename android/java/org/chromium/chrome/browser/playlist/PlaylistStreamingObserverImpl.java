/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistStreamingObserver;

public class PlaylistStreamingObserverImpl implements PlaylistStreamingObserver {
    public interface PlaylistStreamingObserverImplDelegate {
        default void onResponseStarted(String url, long contentLength) {}

        default void onDataReceived(byte[] dataReceived) {}

        default void onDataCompleted() {}
    }

    private PlaylistStreamingObserverImplDelegate mDelegate;

    public PlaylistStreamingObserverImpl() {}

    public PlaylistStreamingObserverImpl(PlaylistStreamingObserverImplDelegate delegate) {
        mDelegate = delegate;
    }

    public PlaylistStreamingObserverImplDelegate getDelegate() {
        return mDelegate;
    }

    public void setDelegate(
            PlaylistStreamingObserverImplDelegate playlistStreamingObserverImplDelegate) {
        mDelegate = playlistStreamingObserverImplDelegate;
    }

    @Override
    public void onResponseStarted(String url, long contentLength) {
        if (mDelegate == null) return;
        mDelegate.onResponseStarted(url, contentLength);
    }

    @Override
    public void onDataReceived(byte[] dataReceived) {
        if (mDelegate == null) return;
        mDelegate.onDataReceived(dataReceived);
    }

    @Override
    public void onDataCompleted() {
        if (mDelegate == null) return;
        mDelegate.onDataCompleted();
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

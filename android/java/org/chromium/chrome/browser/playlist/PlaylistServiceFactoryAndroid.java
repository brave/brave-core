/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.playlist.mojom.PlaylistService;

@JNINamespace("chrome::android")
public class PlaylistServiceFactoryAndroid {
    private static final Object sLock = new Object();
    private static PlaylistServiceFactoryAndroid sInstance;

    public static PlaylistServiceFactoryAndroid getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new PlaylistServiceFactoryAndroid();
            }
        }
        return sInstance;
    }

    private PlaylistServiceFactoryAndroid() {}

    public PlaylistService getPlaylistService(
            Profile profile, ConnectionErrorHandler connectionErrorHandler) {
        if (profile == null) {
            return null;
        }
        long nativeHandle =
                PlaylistServiceFactoryAndroidJni.get().getInterfaceToPlaylistService(profile);
        if (nativeHandle == -1) {
            return null;
        }
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        PlaylistService playlistService = PlaylistService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) playlistService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return playlistService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToPlaylistService(Profile profile);
    }
}

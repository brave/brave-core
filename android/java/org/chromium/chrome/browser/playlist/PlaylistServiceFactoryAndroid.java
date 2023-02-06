/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.playlist.mojom.PlaylistService;

@JNINamespace("chrome::android")
public class PlaylistServiceFactoryAndroid {
    private static final Object lock = new Object();
    private static PlaylistServiceFactoryAndroid instance;

    public static PlaylistServiceFactoryAndroid getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new PlaylistServiceFactoryAndroid();
            }
        }
        return instance;
    }

    private PlaylistServiceFactoryAndroid() {}

    public PlaylistService getPlaylistService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // Always use regular profile
        if (profile == null) {
            return null;
        }
        int nativeHandle =
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

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToPlaylistService(Profile profile);
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.playlist.mojom.PageHandler;

@JNINamespace("chrome::android")
public class PlaylistPageHandlerFactory {
    private static final Object lock = new Object();
    private static PlaylistPageHandlerFactory instance;

    public static PlaylistPageHandlerFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new PlaylistPageHandlerFactory();
            }
        }
        return instance;
    }

    private PlaylistPageHandlerFactory() {}

    public PageHandler getPlaylistPageHandler(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // Always use regular profile
        if (profile == null) {
            return null;
        }
        int nativeHandle =
                PlaylistPageHandlerFactoryJni.get().getInterfaceToPlaylistPageHandler(profile);
        if (nativeHandle == -1) {
            return null;
        }
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        PageHandler pageHandler = PageHandler.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) pageHandler).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return pageHandler;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToPlaylistPageHandler(Profile profile);
    }
}

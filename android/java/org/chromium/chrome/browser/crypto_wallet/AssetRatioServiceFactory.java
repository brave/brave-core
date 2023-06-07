/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class AssetRatioServiceFactory {
    private static final Object lock = new Object();
    private static AssetRatioServiceFactory instance;

    public static AssetRatioServiceFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new AssetRatioServiceFactory();
            }
        }
        return instance;
    }

    private AssetRatioServiceFactory() {}

    public AssetRatioService getAssetRatioService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                AssetRatioServiceFactoryJni.get().getInterfaceToAssetRatioService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        AssetRatioService assetRatioService = AssetRatioService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) assetRatioService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return assetRatioService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToAssetRatioService(Profile profile);
    }
}

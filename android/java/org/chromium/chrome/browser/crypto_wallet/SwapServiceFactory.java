/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class SwapServiceFactory {
    private static final Object sLock = new Object();
    private static SwapServiceFactory sInstance;

    public static SwapServiceFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new SwapServiceFactory();
            }
        }
        return sInstance;
    }

    private SwapServiceFactory() {}

    public SwapService getSwapService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle = SwapServiceFactoryJni.get().getInterfaceToSwapService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        SwapService swapService = SwapService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) swapService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return swapService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToSwapService(Profile profile);
    }
}

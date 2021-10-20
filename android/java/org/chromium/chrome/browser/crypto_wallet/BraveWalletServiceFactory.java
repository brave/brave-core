/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class BraveWalletServiceFactory {
    private static final Object sLock = new Object();
    private static BraveWalletServiceFactory sInstance;

    public static BraveWalletServiceFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveWalletServiceFactory();
            }
        }
        return sInstance;
    }

    private BraveWalletServiceFactory() {}

    public BraveWalletService getBraveWalletService(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = BraveWalletServiceFactoryJni.get().getInterfaceToBraveWalletService();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        BraveWalletService braveWalletService = BraveWalletService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) braveWalletService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return braveWalletService;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToBraveWalletService();
    }
}

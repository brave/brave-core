/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.SwapController;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class SwapControllerFactory {
    private static final Object sLock = new Object();
    private static SwapControllerFactory sInstance;

    public static SwapControllerFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new SwapControllerFactory();
            }
        }
        return sInstance;
    }

    private SwapControllerFactory() {}

    public SwapController getSwapController(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = SwapControllerFactoryJni.get().getInterfaceToSwapController();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        SwapController swapController = SwapController.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) swapController).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return swapController;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToSwapController();
    }
}

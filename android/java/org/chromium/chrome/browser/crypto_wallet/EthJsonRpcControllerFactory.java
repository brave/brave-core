/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class EthJsonRpcControllerFactory {
    private static final Object lock = new Object();
    private static EthJsonRpcControllerFactory instance;

    public static EthJsonRpcControllerFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new EthJsonRpcControllerFactory();
            }
        }
        return instance;
    }

    private EthJsonRpcControllerFactory() {}

    public EthJsonRpcController getEthJsonRpcController(
            ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle =
                EthJsonRpcControllerFactoryJni.get().getInterfaceToEthJsonRpcController();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        EthJsonRpcController ethJsonRpcController =
                EthJsonRpcController.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) ethJsonRpcController).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return ethJsonRpcController;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToEthJsonRpcController();
    }
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class EthTxControllerFactory {
    private static final Object lock = new Object();
    private static EthTxControllerFactory instance;

    public static EthTxControllerFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new EthTxControllerFactory();
            }
        }
        return instance;
    }

    private EthTxControllerFactory() {}

    public EthTxController getEthTxController(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = EthTxControllerFactoryJni.get().getInterfaceToEthTxController();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        EthTxController ethTxController = EthTxController.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) ethTxController).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return ethTxController;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToEthTxController();
    }
}

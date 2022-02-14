/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class TxServiceFactory {
    private static final Object lock = new Object();
    private static TxServiceFactory instance;

    public static TxServiceFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new TxServiceFactory();
            }
        }
        return instance;
    }

    private TxServiceFactory() {}

    public TxService getTxService(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = TxServiceFactoryJni.get().getInterfaceToTxService();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        TxService txService = TxService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) txService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return txService;
    }

    public EthTxManagerProxy getEthTxManagerProxy(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = TxServiceFactoryJni.get().getInterfaceToEthTxManagerProxy();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        EthTxManagerProxy ethTxManagerProxy = EthTxManagerProxy.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) ethTxManagerProxy).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return ethTxManagerProxy;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToTxService();
        int getInterfaceToEthTxManagerProxy();
    }
}

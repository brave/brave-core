/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
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
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle = TxServiceFactoryJni.get().getInterfaceToTxService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        TxService txService = TxService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) txService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return txService;
    }

    public EthTxManagerProxy getEthTxManagerProxy(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle = TxServiceFactoryJni.get().getInterfaceToEthTxManagerProxy(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        EthTxManagerProxy ethTxManagerProxy = EthTxManagerProxy.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) ethTxManagerProxy).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return ethTxManagerProxy;
    }

    public SolanaTxManagerProxy getSolanaTxManagerProxy(
            ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle = TxServiceFactoryJni.get().getInterfaceToSolanaTxManagerProxy(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        SolanaTxManagerProxy solanaTxManagerProxy =
                SolanaTxManagerProxy.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) solanaTxManagerProxy).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return solanaTxManagerProxy;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToTxService(Profile profile);
        long getInterfaceToEthTxManagerProxy(Profile profile);
        long getInterfaceToSolanaTxManagerProxy(Profile profile);
    }
}

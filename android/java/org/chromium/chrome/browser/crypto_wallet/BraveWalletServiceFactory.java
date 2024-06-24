/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
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
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                BraveWalletServiceFactoryJni.get().getInterfaceToBraveWalletService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        BraveWalletService braveWalletService = BraveWalletService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) braveWalletService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return braveWalletService;
    }

    public BraveWalletP3a getBraveWalletP3A(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                BraveWalletServiceFactoryJni.get().getInterfaceToBraveWalletP3A(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        BraveWalletP3a braveWalletP3A = BraveWalletP3a.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) braveWalletP3A).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return braveWalletP3A;
    }

    public JsonRpcService getJsonRpcService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                BraveWalletServiceFactoryJni.get().getInterfaceToJsonRpcService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        JsonRpcService jsonRpcService = JsonRpcService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) jsonRpcService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return jsonRpcService;
    }

    public KeyringService getKeyringService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // Always use regular profile
        long nativeHandle =
                BraveWalletServiceFactoryJni.get().getInterfaceToKeyringService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        KeyringService keyringService = KeyringService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) keyringService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return keyringService;
    }

    public TxService getTxService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle = BraveWalletServiceFactoryJni.get().getInterfaceToTxService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        TxService txService = TxService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) txService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return txService;
    }

    public EthTxManagerProxy getEthTxManagerProxy(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                BraveWalletServiceFactoryJni.get().getInterfaceToEthTxManagerProxy(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        EthTxManagerProxy ethTxManagerProxy = EthTxManagerProxy.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) ethTxManagerProxy).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return ethTxManagerProxy;
    }

    public SolanaTxManagerProxy getSolanaTxManagerProxy(
            ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                BraveWalletServiceFactoryJni.get().getInterfaceToSolanaTxManagerProxy(profile);
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
        long getInterfaceToBraveWalletService(Profile profile);

        long getInterfaceToBraveWalletP3A(Profile profile);

        long getInterfaceToJsonRpcService(Profile profile);

        long getInterfaceToKeyringService(Profile profile);

        long getInterfaceToTxService(Profile profile);

        long getInterfaceToEthTxManagerProxy(Profile profile);

        long getInterfaceToSolanaTxManagerProxy(Profile profile);
    }
}

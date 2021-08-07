/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.ErcTokenRegistry;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class ERCTokenRegistryFactory {
    private static final Object lock = new Object();
    private static ERCTokenRegistryFactory instance;

    public static ERCTokenRegistryFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new ERCTokenRegistryFactory();
            }
        }
        return instance;
    }

    private ERCTokenRegistryFactory() {}

    public ErcTokenRegistry getERCTokenRegistry(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = ERCTokenRegistryFactoryJni.get().getInterfaceToERCTokenRegistry();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        ErcTokenRegistry ercTokenRegistry = ErcTokenRegistry.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) ercTokenRegistry).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return ercTokenRegistry;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToERCTokenRegistry();
    }
}

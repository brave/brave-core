/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class KeyringControllerFactory {
    private static final Object lock = new Object();
    private static KeyringControllerFactory instance;

    public static KeyringControllerFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new KeyringControllerFactory();
            }
        }
        return instance;
    }

    private KeyringControllerFactory() {}

    public KeyringController getKeyringController(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = KeyringControllerFactoryJni.get().getInterfaceToKeyringController();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        KeyringController keyringController = KeyringController.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) keyringController).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return keyringController;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToKeyringController();
    }
}

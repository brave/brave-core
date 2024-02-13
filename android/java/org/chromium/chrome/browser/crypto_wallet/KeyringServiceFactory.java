/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class KeyringServiceFactory {
    private static final Object lock = new Object();
    private static KeyringServiceFactory instance;

    public static KeyringServiceFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new KeyringServiceFactory();
            }
        }
        return instance;
    }

    public KeyringService getKeyringService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // Always use regular profile
        long nativeHandle = KeyringServiceFactoryJni.get().getInterfaceToKeyringService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        KeyringService keyringService = KeyringService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) keyringService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return keyringService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToKeyringService(Profile profile);
    }
}

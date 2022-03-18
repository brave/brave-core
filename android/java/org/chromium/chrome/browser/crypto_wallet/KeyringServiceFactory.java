/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class KeyringServiceFactory {
    private static final Object lock = new Object();
    private static KeyringServiceFactory instance;
    private Profile mProfile;

    public static KeyringServiceFactory getInstance(Profile profile) {
        synchronized (lock) {
            if (instance == null) {
                instance = new KeyringServiceFactory(profile);
            }
        }
        return instance;
    }

    public KeyringServiceFactory(Profile profile) {
        mProfile = profile;
        if (mProfile == null) {
            mProfile = Profile.getLastUsedRegularProfile();
        }
    }

    public KeyringService getKeyringService(ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = KeyringServiceFactoryJni.get().getInterfaceToKeyringService(mProfile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        KeyringService keyringService = KeyringService.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) keyringService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return keyringService;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToKeyringService(Profile profile);
    }
}

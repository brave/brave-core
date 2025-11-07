/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_account;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.brave_account.mojom.Authentication;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
@NullMarked
public class BraveAccountServiceFactory {
    private static final Object sLock = new Object();
    private static @Nullable BraveAccountServiceFactory sInstance;

    public static BraveAccountServiceFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveAccountServiceFactory();
            }
        }
        return sInstance;
    }

    private BraveAccountServiceFactory() {}

    public @Nullable Authentication getBraveAccountService(
            Profile profile, @Nullable ConnectionErrorHandler connectionErrorHandler) {
        long nativeHandle =
                BraveAccountServiceFactoryJni.get().getInterfaceToBraveAccountService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        if (!handle.isValid()) {
            return null;
        }
        Authentication authentication = Authentication.MANAGER.attachProxy(handle, 0);
        if (connectionErrorHandler != null) {
            Handler handler = ((Interface.Proxy) authentication).getProxyHandler();
            handler.setErrorHandler(connectionErrorHandler);
        }

        return authentication;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToBraveAccountService(Profile profile);
    }
}

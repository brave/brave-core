/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.skus;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.skus.mojom.SkusService;

@NullMarked
@JNINamespace("brave::android")
public class SkusServiceFactory {
    private static class LazyHolder {
        static final SkusServiceFactory INSTANCE = new SkusServiceFactory();
    }

    public static SkusServiceFactory getInstance() {
        return LazyHolder.INSTANCE;
    }

    private SkusServiceFactory() {}

    public @Nullable SkusService getSkusService(
            Profile profile, @Nullable ConnectionErrorHandler connectionErrorHandler) {
        long nativeHandle = SkusServiceFactoryJni.get().getInterfaceToSkusService(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        if (!handle.isValid()) {
            return null;
        }
        SkusService skusService = SkusService.MANAGER.attachProxy(handle, 0);
        if (connectionErrorHandler != null && skusService != null) {
            Handler handler = ((Interface.Proxy) skusService).getProxyHandler();
            handler.setErrorHandler(connectionErrorHandler);
        }

        return skusService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToSkusService(Profile profile);
    }
}

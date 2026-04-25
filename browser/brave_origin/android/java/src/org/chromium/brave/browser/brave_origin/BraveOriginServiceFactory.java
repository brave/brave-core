/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.brave_origin;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.brave_origin.mojom.BraveOriginSettingsHandler;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@NullMarked
@JNINamespace("brave::android")
public class BraveOriginServiceFactory {
    private static class LazyHolder {
        static final BraveOriginServiceFactory INSTANCE = new BraveOriginServiceFactory();
    }

    public static BraveOriginServiceFactory getInstance() {
        return LazyHolder.INSTANCE;
    }

    private BraveOriginServiceFactory() {}

    public @Nullable BraveOriginSettingsHandler getBraveOriginSettingsHandler(
            Profile profile, @Nullable ConnectionErrorHandler connectionErrorHandler) {
        long nativeHandle =
                BraveOriginServiceFactoryJni.get()
                        .getInterfaceToBraveOriginSettingsHandler(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        if (!handle.isValid()) {
            return null;
        }
        BraveOriginSettingsHandler braveOriginHandler =
                BraveOriginSettingsHandler.MANAGER.attachProxy(handle, 0);
        if (connectionErrorHandler != null && braveOriginHandler != null) {
            Handler handler = ((Interface.Proxy) braveOriginHandler).getProxyHandler();
            handler.setErrorHandler(connectionErrorHandler);
        }

        return braveOriginHandler;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToBraveOriginSettingsHandler(Profile profile);
    }
}

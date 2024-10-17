/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.webcompat_reporter;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.webcompat_reporter.mojom.WebcompatReporterHandler;

@JNINamespace("chrome::android")
public class WebcompatReporterServiceFactory {
    private static final Object sLock = new Object();
    private static WebcompatReporterServiceFactory sInstance;

    public static WebcompatReporterServiceFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new WebcompatReporterServiceFactory();
            }
        }
        return sInstance;
    }

    private WebcompatReporterServiceFactory() {}

    public WebcompatReporterHandler getWebcompatReporterHandler(
            ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // Always use regular profile
        long nativeHandle =
                WebcompatReporterServiceFactoryJni.get()
                        .getInterfaceToWebcompatReporterService(profile);
        if (nativeHandle == -1) {
            return null;
        }
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        WebcompatReporterHandler webcompatReporterHandler =
                WebcompatReporterHandler.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) webcompatReporterHandler).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return webcompatReporterHandler;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToWebcompatReporterService(Profile profile);
    }
}

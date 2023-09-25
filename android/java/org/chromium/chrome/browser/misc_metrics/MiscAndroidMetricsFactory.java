/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.misc_metrics;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class MiscAndroidMetricsFactory {
    private static final Object lock = new Object();
    private static MiscAndroidMetricsFactory instance;

    public static MiscAndroidMetricsFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new MiscAndroidMetricsFactory();
            }
        }
        return instance;
    }

    private MiscAndroidMetricsFactory() {}

    public MiscAndroidMetrics getMetricsService(ConnectionErrorHandler connectionErrorHandler) {
        Profile profile = Utils.getProfile(false); // always use regular profile
        long nativeHandle =
                MiscAndroidMetricsFactoryJni.get().getInterfaceToMiscAndroidMetrics(profile);
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        MiscAndroidMetrics metricsService = MiscAndroidMetrics.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) metricsService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return metricsService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToMiscAndroidMetrics(Profile profile);
    }
}

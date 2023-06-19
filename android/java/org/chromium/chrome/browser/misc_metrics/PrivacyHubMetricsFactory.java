/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.misc_metrics;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.misc_metrics.mojom.PrivacyHubMetrics;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class PrivacyHubMetricsFactory {
    private static final Object lock = new Object();
    private static PrivacyHubMetricsFactory instance;

    public static PrivacyHubMetricsFactory getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new PrivacyHubMetricsFactory();
            }
        }
        return instance;
    }

    private PrivacyHubMetricsFactory() {}

    public PrivacyHubMetrics getMetricsService(ConnectionErrorHandler connectionErrorHandler) {
        long nativeHandle = PrivacyHubMetricsFactoryJni.get().getInterfaceToPrivacyHubMetrics();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        PrivacyHubMetrics metricsService = PrivacyHubMetrics.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) metricsService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return metricsService;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToPrivacyHubMetrics();
    }
}

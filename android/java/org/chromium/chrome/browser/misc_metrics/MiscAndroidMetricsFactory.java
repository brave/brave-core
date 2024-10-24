/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.misc_metrics;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Promise;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskRunner;
import org.chromium.base.task.TaskTraits;
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
    private static final Object sLock = new Object();
    private static MiscAndroidMetricsFactory sInstance;
    private final TaskRunner mTaskRunner;

    public static MiscAndroidMetricsFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new MiscAndroidMetricsFactory();
            }
        }
        return sInstance;
    }

    private MiscAndroidMetricsFactory() {
        mTaskRunner = PostTask.createSequencedTaskRunner(TaskTraits.UI_DEFAULT);
    }

    public Promise<MiscAndroidMetrics> getMetricsService(
            ConnectionErrorHandler connectionErrorHandler) {
        final Promise<MiscAndroidMetrics> promise = new Promise<>();

        mTaskRunner.execute(
                () -> {
                    Profile profile = Utils.getProfile(false); // always use regular profile
                    long nativeHandle =
                            MiscAndroidMetricsFactoryJni.get()
                                    .getInterfaceToMiscAndroidMetrics(profile);
                    MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
                    MiscAndroidMetrics metricsService =
                            MiscAndroidMetrics.MANAGER.attachProxy(handle, 0);
                    Handler handler = ((Interface.Proxy) metricsService).getProxyHandler();
                    handler.setErrorHandler(connectionErrorHandler);

                    promise.fulfill(metricsService);
                });

        return promise;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToMiscAndroidMetrics(Profile profile);
    }
}

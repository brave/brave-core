/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Promise;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskRunner;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
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
public class BraveNewsControllerFactory {
    private static final Object sLock = new Object();
    private static @Nullable BraveNewsControllerFactory sInstance;
    private final TaskRunner mTaskRunner;

    public static BraveNewsControllerFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveNewsControllerFactory();
            }
        }
        return sInstance;
    }

    private BraveNewsControllerFactory() {
        mTaskRunner = PostTask.createSequencedTaskRunner(TaskTraits.UI_DEFAULT);
    }

    public Promise<BraveNewsController> getBraveNewsController(
            Profile profile, ConnectionErrorHandler connectionErrorHandler) {
        final Promise<BraveNewsController> promise = new Promise<>();

        mTaskRunner.execute(
                () -> {
                    long nativeHandle =
                            BraveNewsControllerFactoryJni.get()
                                    .getInterfaceToBraveNewsController(profile);
                    MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
                    BraveNewsController braveNewsController =
                            BraveNewsController.MANAGER.attachProxy(handle, 0);
                    Handler handler = ((Interface.Proxy) braveNewsController).getProxyHandler();
                    handler.setErrorHandler(connectionErrorHandler);

                    promise.fulfill(braveNewsController);
                });

        return promise;
    }

    private MessagePipeHandle wrapNativeHandle(long nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        long getInterfaceToBraveNewsController(Profile profile);
    }
}

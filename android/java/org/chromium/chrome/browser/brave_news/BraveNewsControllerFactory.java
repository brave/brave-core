/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

@JNINamespace("chrome::android")
public class BraveNewsControllerFactory {
    private static final Object sLock = new Object();
    private static BraveNewsControllerFactory sInstance;

    public static BraveNewsControllerFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveNewsControllerFactory();
            }
        }
        return sInstance;
    }

    private BraveNewsControllerFactory() {}

    public BraveNewsController getBraveNewsController(
            ConnectionErrorHandler connectionErrorHandler) {
        int nativeHandle = BraveNewsControllerFactoryJni.get().getInterfaceToBraveNewsController();
        MessagePipeHandle handle = wrapNativeHandle(nativeHandle);
        BraveNewsController braveNewsController =
                BraveNewsController.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) braveNewsController).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);

        return braveNewsController;
    }

    private MessagePipeHandle wrapNativeHandle(int nativeHandle) {
        return CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
    }

    @NativeMethods
    interface Natives {
        int getInterfaceToBraveNewsController();
    }
}

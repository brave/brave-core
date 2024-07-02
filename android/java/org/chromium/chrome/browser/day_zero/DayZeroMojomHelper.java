/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.day_zero;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.day_zero.mojom.DayZeroBrowserUiExpt;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

/** Helper to interact with native DayZeroBrowserUiExpt. */
@JNINamespace("day_zero")
public class DayZeroMojomHelper implements ConnectionErrorHandler {
    private long mNativeDayZeroBrowserUiExpt;
    DayZeroBrowserUiExpt mDayZeroBroserUiExptHelper;

    private static final Object sLock = new Object();
    private static DayZeroMojomHelper sInstance;

    public static DayZeroMojomHelper getInstance(BrowserContextHandle browserContextHandle) {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new DayZeroMojomHelper(browserContextHandle);
            }
        }
        return sInstance;
    }

    private DayZeroMojomHelper(BrowserContextHandle browserContextHandle) {
        mNativeDayZeroBrowserUiExpt = DayZeroMojomHelperJni.get().init(browserContextHandle);
        long nativeHandle =
                DayZeroMojomHelperJni.get()
                        .getInterfaceToAndroidHelper(mNativeDayZeroBrowserUiExpt);
        MessagePipeHandle handle =
                CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
        mDayZeroBroserUiExptHelper = DayZeroBrowserUiExpt.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) mDayZeroBroserUiExptHelper).getProxyHandler();
        handler.setErrorHandler(this);
    }

    private void destroy() {
        synchronized (sLock) {
            sInstance = null;
            if (mNativeDayZeroBrowserUiExpt == 0 && mDayZeroBroserUiExptHelper == null) {
                return;
            }
            mDayZeroBroserUiExptHelper.close();
            mDayZeroBroserUiExptHelper = null;
            DayZeroMojomHelperJni.get().destroy(mNativeDayZeroBrowserUiExpt);
            mNativeDayZeroBrowserUiExpt = 0;
        }
    }

    public void isDayZeroExpt(DayZeroBrowserUiExpt.IsDayZeroExpt_Response callback) {
        mDayZeroBroserUiExptHelper.isDayZeroExpt(callback);
    }

    @Override
    public void onConnectionError(MojoException e) {
        destroy();
    }

    @NativeMethods
    public interface Natives {
        long init(BrowserContextHandle browserContextHandle);

        void destroy(long nativeDayZeroBrowserUiExpt);

        long getInterfaceToAndroidHelper(long nativeDayZeroBrowserUiExpt);
    }
}

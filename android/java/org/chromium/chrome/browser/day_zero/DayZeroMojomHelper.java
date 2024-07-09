/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.day_zero;

import org.jni_zero.CalledByNative;

import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Helper to interact with native DayZeroBrowserUiExpt. */
// @JNINamespace("day_zero")
// public class DayZeroMojomHelper implements ConnectionErrorHandler {
//     private long mNativeDayZeroBrowserUiExpt;
//     DayZeroBrowserUiExpt mDayZeroBroserUiExptHelper;

//     private static final Object sLock = new Object();
//     private static DayZeroMojomHelper sInstance;

//     public static DayZeroMojomHelper getInstance(BrowserContextHandle browserContextHandle) {
//         synchronized (sLock) {
//             if (sInstance == null) {
//                 sInstance = new DayZeroMojomHelper(browserContextHandle);
//             }
//         }
//         return sInstance;
//     }

//     private DayZeroMojomHelper(BrowserContextHandle browserContextHandle) {
//         mNativeDayZeroBrowserUiExpt = DayZeroMojomHelperJni.get().init(browserContextHandle);
//         long nativeHandle =
//                 DayZeroMojomHelperJni.get()
//                         .getInterfaceToAndroidHelper(mNativeDayZeroBrowserUiExpt);
//         MessagePipeHandle handle =
//                 CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
//         mDayZeroBroserUiExptHelper = DayZeroBrowserUiExpt.MANAGER.attachProxy(handle, 0);
//         Handler handler = ((Interface.Proxy) mDayZeroBroserUiExptHelper).getProxyHandler();
//         handler.setErrorHandler(this);
//     }

//     // @CalledByNative
//     // public static setDayZeroExptAndroid() {
//     //     Log.e("NTP", "DayZeroMojomHelper : setDayZeroExptAndroid : ");
//     // }

//     private void destroy() {
//         synchronized (sLock) {
//             sInstance = null;
//             if (mNativeDayZeroBrowserUiExpt == 0 && mDayZeroBroserUiExptHelper == null) {
//                 return;
//             }
//             mDayZeroBroserUiExptHelper.close();
//             mDayZeroBroserUiExptHelper = null;
//             DayZeroMojomHelperJni.get().destroy(mNativeDayZeroBrowserUiExpt);
//             mNativeDayZeroBrowserUiExpt = 0;
//         }
//     }

//     public void isDayZeroExpt(DayZeroBrowserUiExpt.IsDayZeroExpt_Response callback) {
//         mDayZeroBroserUiExptHelper.isDayZeroExpt(callback);
//     }

//     @Override
//     public void onConnectionError(MojoException e) {
//         destroy();
//     }

//     @NativeMethods
//     public interface Natives {
//         long init(BrowserContextHandle browserContextHandle);

//         void destroy(long nativeDayZeroBrowserUiExpt);

//         long getInterfaceToAndroidHelper(long nativeDayZeroBrowserUiExpt);
//     }
// }

public class DayZeroMojomHelper {
    @CalledByNative
    private static void setDayZeroExptAndroid(boolean isPartOfDayZeroExpt) {
        Log.e("NTP", "DayZeroMojomHelper : setDayZeroExptAndroid : " + isPartOfDayZeroExpt);
        ChromeSharedPreferences.getInstance()
                .writeBoolean("day_zero_expt_flag", isPartOfDayZeroExpt);
    }

    public static boolean getDayZeroExptFlag() {
        Log.e(
                "NTP",
                "getDayZeroExptFlag : "
                        + ChromeSharedPreferences.getInstance()
                                .readBoolean("day_zero_expt_flag", true));
        return ChromeSharedPreferences.getInstance().readBoolean("day_zero_expt_flag", true);
    }
}

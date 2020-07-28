/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import java.lang.reflect.Method;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.BraveConfig;

// The purpose of this class is to hide BraveSyncWorker object under `enable_brave_sync`
// and create it without explict import
public class BraveSyncReflectionUtils {

    private static Object sBraveSyncWorker = null;
    private static boolean sInitialized = false;
    private static String TAG = "SYNC";

    public static Object getSyncWorker() {
        // May be invoked in non-UI thread when we do validation for camera QR in callback
        if (!sInitialized) {
            if (BraveConfig.SYNC_ENABLED) {
                try {
                    sBraveSyncWorker =
                        Class.forName("org.chromium.chrome.browser.BraveSyncWorker")
                             .getConstructor()
                             .newInstance();
                } catch (Exception e) {
                    Log.e(TAG, "Cannot create BraveSyncWorker ", e);
                }
            }
            sInitialized = true;
        }
        return sBraveSyncWorker;
    }

    public static void showInformers() {
Log.e(TAG, "showInformers 000");
        if (!BraveConfig.SYNC_ENABLED) {
Log.e(TAG, "showInformers !BraveConfig.SYNC_ENABLED");
            return;
        }
Log.e(TAG, "showInformers will show");
        try {
            Method method = Class.forName("org.chromium.chrome.browser.BraveSyncInformers").getDeclaredMethod("show");
            method.invoke(null);
        } catch (Exception e) {
            Log.e(TAG, "Cannot show sync informers with reflection ", e);
        }
    }

    // public static boolean isSyncEnabledThroughConfig() {
    //     if (BraveConfig.SYNC_ENABLED) {
    //         return true;
    //     } else {
    //         return false;
    //     }
    // }

//     public static boolean getSyncV1WasEnabled() {
// Log.e(TAG, "BraveSyncWorkerHolder.getSyncV1WasEnabled 000++");
//         Object braveSyncWorker = get();
// Log.e(TAG, "BraveSyncWorkerHolder.getSyncV1WasEnabled braveSyncWorker="+braveSyncWorker);
//         if (braveSyncWorker == null) return false;
//
// //         Class<?> c = braveSyncWorker.getClass();
// // Log.e(TAG, "BraveSyncWorkerHolder.getSyncV1WasEnabled c.getName()="+c.getName());
// //         Method[] allMethods = c.getDeclaredMethods();
// //         for (Method m : allMethods) {
// // Log.e(TAG, "BraveSyncWorkerHolder.getSyncV1WasEnabled m.getName()="+m.getName());
// //         }
// //
// // Log.e(TAG, "BraveSyncWorkerHolder.getSyncV1WasEnabled---------------");
// //         allMethods = c.getMethods();
// //         for (Method m : allMethods) {
// // Log.e(TAG, "BraveSyncWorkerHolder.getSyncV1WasEnabled getMethods m.getName()="+m.getName());
// //         }
//
//         try {
//             java.lang.reflect.Method method = braveSyncWorker.getClass().getDeclaredMethod("getSyncV1WasEnabled");
//             Object ret = method.invoke(braveSyncWorker);
//             return (boolean)ret;
//         } catch (Exception e) {
//             Log.e(TAG, "Cannot make call with reflection ", e);
//         }
//         return false;
//     }
//
//     public static boolean getSyncV2MigrateNoticeDismissed() {
// Log.e(TAG, "BraveSyncWorkerHolder.getSyncV2MigrateNoticeDismissed 000");
//         Object braveSyncWorker = get();
//         if (braveSyncWorker == null) return false;
//
//         java.lang.reflect.Method method;
//         try {
//             method = braveSyncWorker.getClass().getMethod("getSyncV2MigrateNoticeDismissed");
//             Object ret = method.invoke(braveSyncWorker);
//             return (boolean)ret;
//         } catch (Exception e) {
//             Log.e(TAG, "Cannot make call with reflection ", e);
//         }
//         return false;
//     }
//
//     public static void setSyncV2MigrateNoticeDismissed(boolean isDismissed) {
//     Log.e(TAG, "BraveSyncWorkerHolder.setSyncV2MigrateNoticeDismissed 000 isDismissed="+isDismissed);
//         Object braveSyncWorker = get();
//         if (braveSyncWorker == null) return;
//
//         java.lang.reflect.Method method;
//         try {
//             method = braveSyncWorker.getClass().getMethod("setSyncV2MigrateNoticeDismissed", boolean.class);
//             method.invoke(braveSyncWorker, isDismissed);
//         } catch (Exception e) {
//             Log.e(TAG, "Cannot make call with reflection ", e);
//         }
//     }
}

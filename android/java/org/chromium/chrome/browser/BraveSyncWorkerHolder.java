/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import java.lang.reflect.InvocationTargetException;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.BraveConfig;

// The purpose of this class is to hide BraveSyncWorker object under `enable_brave_sync`
// and create it without explict import
public class BraveSyncWorkerHolder {

    private static Object sBraveSyncWorker = null;
    private static boolean sInitialized = false;
    private static String TAG = "SYNC";

    public static Object get() {
        // May be invoked in non-UI thread when we do validation for camera QR in callback
        if (!sInitialized) {
            if (BraveConfig.SYNC_ENABLED) {
                try {
                    sBraveSyncWorker =
                        Class.forName("org.chromium.chrome.browser.BraveSyncWorker")
                             .getConstructor()
                             .newInstance();
                } catch (ClassNotFoundException e) {
                    Log.e(TAG, "Cannot create BraveSyncWorker ClassNotFoundException", e);
                } catch (NoSuchMethodException e) {
                    Log.e(TAG, "Cannot create BraveSyncWorker NoSuchMethodException", e);
                } catch (IllegalAccessException e) {
                    Log.e(TAG, "Cannot create BraveSyncWorker IllegalAccessException", e);
                } catch (InstantiationException e) {
                    Log.e(TAG, "Cannot create BraveSyncWorker InstantiationException", e);
                } catch (InvocationTargetException e) {
                    Log.e(TAG, "Cannot create BraveSyncWorker InvocationTargetException", e);
                }
            }
            sInitialized = true;
        }
        return sBraveSyncWorker;
    }
}

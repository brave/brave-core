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

    private static Object sBraveSyncWorker;
    private static boolean sInitialized;
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
        if (!BraveConfig.SYNC_ENABLED) {
            return;
        }

        try {
            Method method = Class.forName("org.chromium.chrome.browser.BraveSyncInformers").getDeclaredMethod("show");
            method.invoke(null);
        } catch (Exception e) {
            Log.e(TAG, "Cannot show sync informers with reflection ", e);
        }
    }
}

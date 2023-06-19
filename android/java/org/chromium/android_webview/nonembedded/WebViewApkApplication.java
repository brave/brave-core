/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.android_webview.nonembedded;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;

public class WebViewApkApplication {
    private static final String TAG = "BraveWebViewApkApp";

    public static void postDeveloperUiLauncherIconTask() {
        wtf();
    }

    // Copied from upstreams's android_webview/nonembedded/WebViewApkApplication.java
    // to ensure we don't create any webview process
    // Returns true if running in the "webview_apk" or "webview_service" process.
    public static boolean isWebViewProcess() {
        // Either "webview_service", or "webview_apk".
        // "webview_service" is meant to be very light-weight and never load the native library.
        boolean hasWebViewPrefix = ContextUtils.getProcessName().contains(":webview_");
        if (hasWebViewPrefix) {
            wtf();
        }
        return hasWebViewPrefix;
    }

    public static void maybeInitProcessGlobals() {
        wtf();
    }

    public static void checkForAppRecovery() {
        wtf();
    }

    // WTF is for `What a Terrible Failure`
    private static void wtf() {
        Log.wtf(TAG, "Android Webview API must not be invoked");
        assert false;
    }
}

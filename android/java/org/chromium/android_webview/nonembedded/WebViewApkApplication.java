/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.android_webview.nonembedded;

import org.chromium.base.Log;

public class WebViewApkApplication {
    private static final String TAG = "BraveWebViewApkApplication";

    public static void postDeveloperUiLauncherIconTask() {
        Log.wtf(TAG, "Android Webview API must not be invoked");
    }
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_account;

import android.app.Activity;

import org.jni_zero.CalledByNative;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/** Opens the Brave Account authentication flows from the WebUI serving the rows. */
@NullMarked
public class BraveAccountDialogLauncherHelper {
    @CalledByNative
    private static void showBraveAccountDialog(WebContents webContents, String url) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (windowAndroid == null) {
            return;
        }

        Activity activity = windowAndroid.getActivity().get();
        if (activity == null) {
            return;
        }

        BraveAccountCustomTabActivity.show(activity, url);
    }
}

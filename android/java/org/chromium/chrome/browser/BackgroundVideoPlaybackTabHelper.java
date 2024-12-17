/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;
import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.content_public.browser.WebContents;

/**
 * @noinspection unused
 */
@JNINamespace("youtube_script_injector")
public class BackgroundVideoPlaybackTabHelper {
    private static final String TAG = "BackgroundVideoPlaybackTabHelper";

    public static void setFullscreen(WebContents webContents) {
        BackgroundVideoPlaybackTabHelperJni.get().setFullscreen(webContents);
    }

    public static boolean isPlayingMedia(WebContents webContents) {
        return BackgroundVideoPlaybackTabHelperJni.get().isPlayingMedia(webContents);
    }

    @NativeMethods
    interface Natives {
        void setFullscreen(WebContents webContents);

        boolean isPlayingMedia(WebContents webContents);
    }

    @CalledByNative
    public static void showYouTubeFeaturesLayout(boolean show) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.showYouTubeFeaturesLayout(show);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showYouTubeFeaturesLayout", e);
        }
    }
}

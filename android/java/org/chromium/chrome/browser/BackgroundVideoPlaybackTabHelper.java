/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.content_public.browser.WebContents;

@JNINamespace("chrome::android")
public class BackgroundVideoPlaybackTabHelper {

    public static void toggleFullscreen(WebContents webContents, boolean isFullScreen) {
        BackgroundVideoPlaybackTabHelperJni.get().toggleFullscreen(webContents, isFullScreen);
    }

    public static boolean isPlayingMedia(WebContents webContents) {
        return BackgroundVideoPlaybackTabHelperJni.get().isPlayingMedia(webContents);
    }

    @NativeMethods
    interface Natives {
        void toggleFullscreen(WebContents webContents, boolean isFullScreen);

        boolean isPlayingMedia(WebContents webContents);
    }
}

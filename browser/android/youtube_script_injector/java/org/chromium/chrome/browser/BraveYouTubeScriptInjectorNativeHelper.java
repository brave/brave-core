/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;
import org.chromium.content_public.browser.WebContents;

/**
 * Helper to interact with native BraveYouTubeScriptInjectorNativeHelper.
 * Check brave_youtube_script_injector_native_helper.{h|cc}, for native parts.
 */
@JNINamespace("youtube_script_injector")
public class BraveYouTubeScriptInjectorNativeHelper {
    public static boolean isYouTubeVideo(WebContents webContents) {
        return BraveYouTubeScriptInjectorNativeHelperJni.get().isYouTubeVideo(webContents);
    }

    public static void setFullscreen(WebContents webContents) {
        BraveYouTubeScriptInjectorNativeHelperJni.get().setFullscreen(webContents);
    }
    /**
     * @noinspection unused
     */
    @NativeMethods
    interface Natives {
        boolean isYouTubeVideo(WebContents webContents);
        void setFullscreen(WebContents webContents);
    }
}

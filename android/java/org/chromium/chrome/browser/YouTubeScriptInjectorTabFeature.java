/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.WebContents;

/**
 * @noinspection unused
 */
@JNINamespace("youtube_script_injector")
public class YouTubeScriptInjectorTabFeature {
    private static final String TAG = "YouTubeTabFeature";

    public static void setFullscreen(WebContents webContents) {
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_YOUTUBE_SCRIPT_INJECTOR)
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_YOUTUBE_EXTRA_CONTROLS)
                && UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getBoolean(BravePref.YOU_TUBE_EXTRA_CONTROLS_ENABLED)) {
            YouTubeScriptInjectorTabFeatureJni.get().setFullscreen(webContents);
        }
    }

    @NativeMethods
    interface Natives {
        void setFullscreen(WebContents webContents);
    }

    @CalledByNative
    public static void enterPipMode() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.enterPipMode();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "enterPipMode", e);
        }
    }
}

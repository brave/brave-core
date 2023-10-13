/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.content.Context;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.settings.BraveLeoPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.content_public.browser.WebContents;

/** Launches Brave Leo settings page. */
public class BraveLeoSettingsLauncherHelper {
    private static SettingsLauncher sLauncher;

    @CalledByNative
    private static void showBraveLeoSettings(WebContents webContents) {
        Context context = webContents.getTopLevelNativeWindow().getActivity().get();
        if (context == null) {
            return;
        }
        getLauncher().launchSettingsActivity(context, BraveLeoPreferences.class);
    }

    private static SettingsLauncher getLauncher() {
        return sLauncher != null ? sLauncher : new SettingsLauncherImpl();
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import org.jni_zero.CalledByNative;

import org.chromium.chrome.browser.settings.BraveLeoPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.content_public.browser.WebContents;

/** Launches Brave Leo settings page or subscription. */
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

    @CalledByNative
    private static void goPremium(WebContents webContents) {
        Activity activity = webContents.getTopLevelNativeWindow().getActivity().get();
        Intent braveLeoPlansIntent = new Intent(activity, BraveLeoPlansActivity.class);
        braveLeoPlansIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        braveLeoPlansIntent.setAction(Intent.ACTION_VIEW);
        activity.startActivity(braveLeoPlansIntent);
    }

    private static SettingsLauncher getLauncher() {
        return sLauncher != null ? sLauncher : new SettingsLauncherImpl();
    }
}

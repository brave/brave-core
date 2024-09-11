/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.app.Activity;
import android.content.Context;

import org.jni_zero.CalledByNative;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveLeoPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherFactory;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/** Launches Brave Leo settings page or subscription. */
public class BraveLeoSettingsLauncherHelper {
    private static final String ACCOUNT_PAGE_URL = "https://account.brave.com/";
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
        BraveLeoUtils.goPremium(webContents.getTopLevelNativeWindow().getActivity().get());
    }

    @CalledByNative
    private static void managePremium(WebContents webContents) {
        if (BraveLeoPrefUtils.getIsSubscriptionActive(Profile.fromWebContents(webContents))) {
            BraveLeoUtils.openManageSubscription();
        } else {
            TabUtils.openURLWithBraveActivity(ACCOUNT_PAGE_URL);
        }
    }

    @CalledByNative
    private static void openURL(String url) {
        TabUtils.openURLWithBraveActivity(url);
    }

    @CalledByNative
    private static void handleVoiceRecognition(
            WebContents chatWindowWebContents, WebContents contextWebContents) {
        new BraveLeoVoiceRecognitionHandler(
                        chatWindowWebContents.getTopLevelNativeWindow(), contextWebContents)
                .startVoiceRecognition();
    }

    @CalledByNative
    private static void closeActivity(WebContents webContents) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (windowAndroid == null) {
            return;
        }
        Activity activity = windowAndroid.getActivity().get();
        if (activity == null) {
            return;
        }
        activity.finish();
    }

    private static SettingsLauncher getLauncher() {
        return sLauncher != null ? sLauncher : SettingsLauncherFactory.createSettingsLauncher();
    }
}

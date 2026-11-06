/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_account;

import android.app.Activity;

import org.jni_zero.CalledByNative;

import org.chromium.brave_account.mojom.DialogMode;
import org.chromium.brave_account.mojom.SettingsPage;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/** Opens the screens that WebUI pages ask for. */
@NullMarked
public class BraveAccountScreenOpener {
    @CalledByNative
    private static boolean openBraveAccountDialog(
            WebContents webContents, @DialogMode.EnumType int dialogMode) {
        Activity activity = getActivity(webContents);
        if (activity == null) {
            return false;
        }

        BraveAccountCustomTabActivity.openBraveAccountDialog(activity, dialogMode);
        return true;
    }

    @CalledByNative
    private static boolean openSettingsPage(
            WebContents webContents, @SettingsPage.EnumType int page) {
        Activity activity = getActivity(webContents);
        if (activity == null) {
            return false;
        }

        switch (page) {
            case SettingsPage.BRAVE_ACCOUNT:
                BraveAccountCustomTabActivity.openBraveAccountSettings(activity);
                return true;
            default:
                return false;
        }
    }

    private static @Nullable Activity getActivity(WebContents webContents) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        return windowAndroid != null ? windowAndroid.getActivity().get() : null;
    }
}

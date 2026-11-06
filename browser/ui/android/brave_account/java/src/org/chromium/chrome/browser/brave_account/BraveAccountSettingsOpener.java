/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_account;

import android.app.Activity;

import org.jni_zero.CalledByNative;

import org.chromium.brave_account.mojom.SettingsPage;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/** Opens a settings page from a WebUI that only links to it. */
@NullMarked
public class BraveAccountSettingsOpener {
    @CalledByNative
    private static boolean openSettingsPage(
            WebContents webContents, @SettingsPage.EnumType int page) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (windowAndroid == null) {
            return false;
        }

        Activity activity = windowAndroid.getActivity().get();
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
}

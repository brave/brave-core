/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.content_public.browser.WebContents;
import org.chromium.mojo.bindings.Callbacks;

@JNINamespace("brave_wallet")
public class BraveWalletProviderDelegateImplHelper {
    @CalledByNative
    public static void showPanel() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.showWalletPanel(false);
        }
    }

    @CalledByNative
    public static void showWalletOnboarding() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.showWalletOnboarding();
        }
    }

    @CalledByNative
    public static void walletInteractionDetected(WebContents webContents) {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.walletInteractionDetected(webContents);
        }
    }

    @CalledByNative
    public static boolean isWeb3NotificationAllowed() {
        return BraveWalletPreferences.getPrefWeb3NotificationsEnabled();
    }

    @CalledByNative
    public static void ShowAccountCreation(String keyringId) {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.showAccountCreation(keyringId);
        }
    }

    public static void IsSolanaConnected(
            WebContents webContents, String account, Callbacks.Callback1<Boolean> callback) {
        Callback<Boolean> callbackWrapper = result -> {
            callback.call(result);
        };
        BraveWalletProviderDelegateImplHelperJni.get().IsSolanaConnected(
                webContents, account, callbackWrapper);
    }

    @NativeMethods
    interface Natives {
        void IsSolanaConnected(WebContents webContents, String account, Callback<Boolean> callback);
    }
}

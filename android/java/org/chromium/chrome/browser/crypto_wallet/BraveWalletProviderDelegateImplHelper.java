/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.content_public.browser.WebContents;
import org.chromium.mojo.bindings.Callbacks;

@JNINamespace("brave_wallet")
public class BraveWalletProviderDelegateImplHelper {
    private static final String TAG = "BraveWalletProvider";

    @CalledByNative
    public static void showPanel() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.showWalletPanel(false);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showPanel " + e);
        }
    }

    @CalledByNative
    public static void showWalletOnboarding() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.showWalletOnboarding();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showWalletOnboarding " + e);
        }
    }

    @CalledByNative
    public static void walletInteractionDetected(WebContents webContents) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.walletInteractionDetected(webContents);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "walletInteractionDetected " + e);
        }
    }

    @CalledByNative
    public static boolean isWeb3NotificationAllowed() {
        return BraveWalletPreferences.getPrefWeb3NotificationsEnabled();
    }

    @CalledByNative
    public static void ShowAccountCreation(@CoinType.EnumType int coinType) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.showAccountCreation(coinType);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "ShowAccountCreation " + e);
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

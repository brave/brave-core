/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.app.BraveActivity;

@JNINamespace("brave_wallet")
public class BraveWalletProviderDelegateImplHelper {
    @CalledByNative
    public static void showPanel() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.showWalletPanel();
        }
    }

    @CalledByNative
    public static void showWalletOnboarding() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.showWalletOnboarding();
        }
    }
}

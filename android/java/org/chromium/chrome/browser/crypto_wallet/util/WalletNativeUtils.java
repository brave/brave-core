/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("chrome::android")
public class WalletNativeUtils {
    public static void resetWallet() {
        WalletNativeUtilsJni.get().resetWallet();
    }

    @NativeMethods
    interface Natives {
        void resetWallet();
    }
}

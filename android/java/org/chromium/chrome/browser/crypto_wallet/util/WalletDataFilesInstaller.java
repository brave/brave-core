/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Class for JNI interaction with wallet_data_files_installer_android.cc
 */
@JNINamespace("chrome::android")
public class WalletDataFilesInstaller {
    public static String getWalletDataFilesComponentId() {
        return WalletDataFilesInstallerJni.get().getWalletDataFilesComponentId();
    }

    @NativeMethods
    interface Natives {
        String getWalletDataFilesComponentId();
    }
}

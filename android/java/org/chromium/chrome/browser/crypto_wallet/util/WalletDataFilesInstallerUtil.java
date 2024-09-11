/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.jni_zero.CalledByNative;

/**
 * Class that is used by wallet_data_files_installer.cc to determine, if we need to download Brave
 * Wallet data files component on Android at startup
 */
public class WalletDataFilesInstallerUtil {
    @CalledByNative
    public static boolean isBraveWalletConfiguredOnAndroid() {
        return !Utils.shouldShowCryptoOnboarding();
    }
}

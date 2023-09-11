/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

public class AddressUtils {
    public static String sanitizeEthAddress(String ethAddress) {
        if (TextUtils.isEmpty(ethAddress)) return "";
        if (ethAddress.matches(WalletConstants.REGX_ANY_ETH_ADDRESS)) {
            return ethAddress.replaceAll(WalletConstants.REGX_ANY_ETH_ADDRESS, "$1");
        }
        return ethAddress;
    }

    /**
     * Gets truncated address from a full contract address.
     * @param address full contract address
     * @return truncated address
     */
    public static String getTruncatedAddress(String address) {
        int prefixLength = address.startsWith("0x") ? 6 : 4;
        return (address.substring(0, prefixLength) + "***"
                + address.substring(address.length() - 4));
    }
}

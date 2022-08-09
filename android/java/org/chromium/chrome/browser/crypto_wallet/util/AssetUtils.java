/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

public class AssetUtils {
    public static String ASSET_SYMBOL_ETH = "ETH";

    public static String AURORA_SUPPORTED_CONTRACT_ADDRESSES[] = {
            "0x4e834cdcc911605227eedddb89fad336ab9dc00a", // AAVE
            "0x8bec47865ade3b172a928df8f990bc7f2a3b9f79", // AURORA
            "0xb59d0fdaf498182ff19c4e80c00ecfc4470926e2", // BAL
            "0x2b9025aecc5ce7a8e6880d3e9c6e458927ecba04", // BAT
            "0xdeacf0faa2b80af41470003b5f6cd113d47b4dcd", // COMP
            "0xabe9818c5fb5e751c4310be6f0f18c8d85f9bd7f", // CREAM
            "0xe3520349f477a5f6eb06107066048508498a291b", // DAI
            "0xe301ed8c7630c9678c39e4e45193d1e7dfb914f7", // DODO
            "0xea62791aa682d455614eaa2a12ba3d9a2fd197af", // FLX
            "0xda2585430fef327ad8ee44af8f1f989a2a91a3d2", // FRAX
            "0xc8fdd32e0bf33f0396a18209188bb8c6fb8747d2", // FXS
            "0x943f4bf75d5854e92140403255a471950ab8a26f", // HAPI
            "0x94190d8ef039c670c6d6b9990142e0ce2a1e3178", // LINK
            "0x1d1f82d8b8fc72f29a8c268285347563cb6cd8b3", // MKR
            "0x74974575d2f1668c63036d51ff48dbaa68e52408", // MODA
            "0x951cfdc9544b726872a8faf56792ef6704731aae", // OCT
            "0x07b2055fbd17b601c780aeb3abf4c2b3a30c7aae", // OIN
            "0x885f8CF6E45bdd3fdcDc644efdcd0AC93880c781", // PAD
            "0x291c8fceaca3342b29cc36171deb98106f712c66", // PICKLE
            "0x18921f1e257038e538ba24d49fa6495c8b1617bc", // REN
            "0xdc9be1ff012d3c6da818d136a3b2e5fdd4442f74", // SNX
            "0x7821c773a12485b12a2b5b7bc451c3eb200986b1", // SUSHI
            "0x1bc741235ec0ee86ad488fa49b69bb6c823ee7b7", // UNI
            "0xb12bfca5a55806aaf64e99521918a4bf0fc40802", // USDC
            "0x4988a896b1227218e4a686fde5eabdcabd91571f", // USDT
            "0xf4eb217ba2454613b15dbdea6e5f22276410e89e", // WBTC
            "0x99ec8f13b2afef5ec49073b9d20df109d25f78c0", // WOO
            "0xa64514a8af3ff7366ad3d5daa5a548eefcef85e0" // YFI
    };

    public static boolean isAuroraAddress(String contractAddress, String assetSymbol) {
        if (!TextUtils.isEmpty(contractAddress)) {
            for (String address : AURORA_SUPPORTED_CONTRACT_ADDRESSES) {
                if (address.equalsIgnoreCase(contractAddress)) {
                    return true;
                }
            }
        }
        return ASSET_SYMBOL_ETH.equalsIgnoreCase(assetSymbol);
    }
}

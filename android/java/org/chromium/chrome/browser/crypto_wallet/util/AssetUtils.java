/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;

public class AssetUtils {
    public static String AURORA_SUPPORTED_CONTRACT_ADDRESSES[] = {
            "0x7fc66500c84a76ad7e9c93437bfc5ac33e2ddae9", // AAVE
            "0xaaaaaa20d9e0e2461697782ef11675f668207961", // AURORA
            "0xba100000625a3754423978a60c9317c58a424e3d", // BAL
            "0x0d8775f648430679a709e98d2b0cb6250d2887ef", // BAT
            "0xc00e94cb662c3520282e6f5717214004a7f26888", // COMP
            "0x2ba592f78db6436527729929aaf6c908497cb200", // CREAM
            "0x6b175474e89094c44da98b954eedeac495271d0f", // DAI
            "0x43dfc4159d86f3a37a5a4b3d4580b888ad7d4ddd", // DODO
            "0x3ea8ea4237344c9931214796d9417af1a1180770", // FLX
            "0x853d955acef822db058eb8505911ed77f175b99e", // FRAX
            "0x3432b6a60d23ca0dfca7761b7ab56459d9c964d0", // FXS
            "0xd9c2d319cd7e6177336b0a9c93c21cb48d84fb54", // HAPI
            "0x514910771af9ca656af840dff83e8264ecf986ca", // LINK
            "0x9f8f72aa9304c8b593d555f12ef6589cc3a579a2", // MKR
            "0x1117ac6ad6cdf1a3bc543bad3b133724620522d5", // MODA
            "0xf5cfbc74057c610c8ef151a439252680ac68c6dc", // OCT
            "0x9aeb50f542050172359a0e1a25a9933bc8c01259", // OIN
            "0xea7cc765ebc94c4805e3bff28d7e4ae48d06468a", // PAD
            "0x429881672b9ae42b8eba0e26cd9c73711b891ca5", // PICKLE
            "0x408e41876cccdc0f92210600ef50372656052a38", // REN
            "0xc011a73ee8576fb46f5e1c5751ca3b9fe0af2a6f", // SNX
            "0x6b3595068778dd592e39a122f4f5a5cf09c90fe2", // SUSHI
            "0x1f9840a85d5af5bf1d1762f925bdaddc4201f984", // UNI
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48", // USDC
            "0xdac17f958d2ee523a2206206994597c13d831ec7", // USDT
            "0x2260fac5e5542a773aa44fbcfedf7c193bc2c599", // WBTC
            "0x4691937a7508860f876c9c0a2a617e7d9e945d4b", // WOO
            "0x0bc529c00c6401aef6d220be8c6ea1667f6ad93e" // YFI
    };

    public static boolean isAuroraAddress(String contractAddress, String chainId) {
        boolean isEthereumBridgeAddress = false;
        boolean isNativeAsset = TextUtils.isEmpty(contractAddress);

        if (!isNativeAsset) {
            for (String address : AURORA_SUPPORTED_CONTRACT_ADDRESSES) {
                isEthereumBridgeAddress = address.equalsIgnoreCase(contractAddress);
                if (isEthereumBridgeAddress) {
                    break;
                }
            }
        }
        return (isEthereumBridgeAddress || isNativeAsset)
                && chainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID);
    }
}

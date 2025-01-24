/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;

public class NetworkUtils {

    public static Comparator<NetworkInfo> sSortNetworkByPriority =
            Comparator.comparingInt(c -> getCoinRank(c.coin));

    // Solana first
    private static int getCoinRank(@CoinType.EnumType int coin) {
        switch (coin) {
            case CoinType.SOL:
                return 0;
            case CoinType.ETH:
                return 1;
            case CoinType.FIL:
                return 2;
            case CoinType.BTC:
                return 3;
            default:
                return Integer.MAX_VALUE;
        }
    }

    public static class Filters {
        public static boolean isSameNetwork(NetworkInfo network, String chainId, int coin) {
            return network.chainId.equals(chainId) && network.coin == coin;
        }

        public static boolean isSameNetwork(NetworkInfo network1, NetworkInfo network2) {
            return isSameNetwork(network1, network2.chainId, network2.coin);
        }

        public static boolean isLocalNetwork(NetworkInfo network) {
            return network.chainId.equals(BraveWalletConstants.LOCALHOST_CHAIN_ID);
        }
    }

    /**
     * Returns {@code true} if two networks are equal.The comparison is done checking chain name,
     * chain ID, coin, symbol and symbol name.
     */
    public static boolean areEqual(
            @NonNull final NetworkInfo network1, @NonNull final NetworkInfo network2) {
        return network1.chainName.equals(network2.chainName)
                && network1.chainId.equals(network2.chainId)
                && network1.coin == network2.coin
                && network1.symbol.equals(network2.symbol)
                && network1.symbolName.equals(network2.symbolName);
    }

    public static boolean isAllNetwork(@Nullable final NetworkInfo networkInfo) {
        if (networkInfo == null) return false;
        return networkInfo.chainId.equals("all");
    }

    @SuppressWarnings("NoStreams")
    public static List<NetworkInfo> nonTestNetwork(List<NetworkInfo> networkInfos) {
        if (networkInfos == null) return Collections.emptyList();
        return networkInfos.stream()
                .filter(network -> !WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(network.chainId))
                .collect(Collectors.toList());
    }

    /**
     * Gets the network info object of given chainId and symbol.
     *
     * @param networks All networks available.
     * @param chainId Chain Id of the network to be found.
     * @param coin Coin type of the network to be found.
     * @return Network info or {@code null} if the network was not found.
     */
    @Nullable
    public static NetworkInfo findNetwork(
            @NonNull List<NetworkInfo> networks, @Nullable String chainId, int coin) {
        if (networks.isEmpty() || TextUtils.isEmpty(chainId)) {
            return null;
        }
        return JavaUtils.find(
                networks,
                networkInfo -> networkInfo.chainId.equals(chainId) && networkInfo.coin == coin);
    }

    public static boolean isTestNetwork(String chainId) {
        return WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(chainId);
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.url.mojom.Url;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;

public class NetworkUtils {
    private static NetworkInfo sAllNetworksOption;

    public static Comparator<NetworkInfo> sSortNetworkByPriority = (c1, c2) -> {
        return Integer.compare(getCoinRank(c1.coin), getCoinRank(c2.coin));
    };

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

    public static NetworkInfo getAllNetworkOption(Context context) {
        if (sAllNetworksOption == null) {
            NetworkInfo allNetworkInfo = new NetworkInfo();
            allNetworkInfo.blockExplorerUrls = new String[0];
            allNetworkInfo.chainId = "all";
            allNetworkInfo.chainName = context.getString(R.string.brave_wallet_network_filter_all);
            allNetworkInfo.coin = 0;
            allNetworkInfo.supportedKeyrings = new int[0];
            allNetworkInfo.decimals = 0;
            allNetworkInfo.iconUrls = new String[0];
            allNetworkInfo.activeRpcEndpointIndex = 0;
            allNetworkInfo.rpcEndpoints = new Url[0];
            allNetworkInfo.symbol = "all";
            allNetworkInfo.symbolName = "all";
            allNetworkInfo.isEip1559 = false;
            sAllNetworksOption = allNetworkInfo;
        }
        return sAllNetworksOption;
    }

    public static boolean isAllNetwork(NetworkInfo networkInfo) {
        if (networkInfo == null) return false;
        return networkInfo.chainId.equals("all");
    }

    public static List<NetworkInfo> nonTestNetwork(List<NetworkInfo> networkInfos) {
        if (networkInfos == null) return Collections.emptyList();
        return networkInfos.stream()
                .filter(network -> !WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(network.chainId))
                .collect(Collectors.toList());
    }

    /**
     * Get the NetworkInfo object of given chainId
     * @param networkInfos all networks
     * @param chainId of network to be found
     * @return found network or null
     */
    public static NetworkInfo findNetwork(List<NetworkInfo> networkInfos, String chainId) {
        if (networkInfos.isEmpty() || TextUtils.isEmpty(chainId)) return null;
        return JavaUtils.find(networkInfos, networkInfo -> networkInfo.chainId.equals(chainId));
    }

    public static boolean isTestNetwork(String chainId) {
        return WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(chainId);
    }
}

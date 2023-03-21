/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.url.mojom.Url;

import java.util.List;

public class NetworkUtils {
    private static NetworkInfo sAllNetworksOption;

    public static NetworkInfo getAllNetworkOption(Context context) {
        if (sAllNetworksOption == null) {
            NetworkInfo allNetworkInfo = new NetworkInfo();
            allNetworkInfo.blockExplorerUrls = new String[0];
            allNetworkInfo.chainId = "all";
            allNetworkInfo.chainName = context.getString(R.string.brave_wallet_network_filter_all);
            allNetworkInfo.coin = 0;
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
}

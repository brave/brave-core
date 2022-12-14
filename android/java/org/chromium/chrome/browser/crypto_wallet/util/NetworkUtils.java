/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.url.mojom.Url;

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
}

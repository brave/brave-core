/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.mojo.bindings.Callbacks;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;

public class NetworkResponsesCollector {
    private JsonRpcService mJsonRpcService;
    private List<Integer> coinTypes;
    private HashSet<NetworkInfo> mNetworks;

    public NetworkResponsesCollector(JsonRpcService jsonRpcService, List<Integer> coinTypes) {
        assert jsonRpcService != null;
        mJsonRpcService = jsonRpcService;
        this.coinTypes = coinTypes;
        mNetworks = new LinkedHashSet<>();
    }

    public void getNetworks(Callbacks.Callback1<HashSet<NetworkInfo>> runWhenDone) {
        AsyncUtils.MultiResponseHandler networkInfosMultiResponse =
                new AsyncUtils.MultiResponseHandler(coinTypes.size());
        ArrayList<AsyncUtils.GetNetworkResponseContext> accountsPermissionsContexts =
                new ArrayList<>();
        for (int coin : coinTypes) {
            AsyncUtils.GetNetworkResponseContext networksContext =
                    new AsyncUtils.GetNetworkResponseContext(
                            networkInfosMultiResponse.singleResponseComplete);

            accountsPermissionsContexts.add(networksContext);

            mJsonRpcService.getAllNetworks(coin, networksContext);
        }

        networkInfosMultiResponse.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetNetworkResponseContext getNetworkResponseContext :
                    accountsPermissionsContexts) {
                if (getNetworkResponseContext.networkInfos.length == 0) {
                    continue;
                }

                mNetworks.addAll(Arrays.asList(getNetworkResponseContext.networkInfos));
            }
            runWhenDone.call(mNetworks);
        });
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class NetworkResponsesCollector {
    private final JsonRpcService mJsonRpcService;
    private final List<Integer> mCoinTypes;
    private final List<NetworkInfo> mNetworks;

    public NetworkResponsesCollector(JsonRpcService jsonRpcService, List<Integer> coinTypes) {
        assert jsonRpcService != null;
        mJsonRpcService = jsonRpcService;
        mCoinTypes = coinTypes;
        mNetworks = new ArrayList<>();
    }

    public void getNetworks(Callbacks.Callback1<List<NetworkInfo>> runWhenDone) {
        AsyncUtils.MultiResponseHandler networkInfosMultiResponse =
                new AsyncUtils.MultiResponseHandler(mCoinTypes.size());
        ArrayList<AsyncUtils.GetNetworkResponseContext> accountsPermissionsContexts =
                new ArrayList<>();
        for (int coin : mCoinTypes) {
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
            Collections.sort(mNetworks, NetworkUtils.sSortNetworkByPriority);
            runWhenDone.call(mNetworks);
        });
    }
}

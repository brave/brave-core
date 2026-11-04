/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.build.annotations.NullMarked;

// Network state for the wallet, resolved from the default network of the selected coin.
@NullMarked
public class WalletNetworkModel extends NetworkModel {
    public WalletNetworkModel(
            BraveWalletService braveWalletService,
            @NonNull JsonRpcService jsonRpcService,
            CryptoSharedData sharedData,
            CryptoSharedActions cryptoSharedActions) {
        super(braveWalletService, jsonRpcService, sharedData, cryptoSharedActions);
    }

    @Override
    void updateChainId() {
        if (mJsonRpcService == null) {
            return;
        }
        @Nullable Integer coinBoxed = mSharedData.getCoinTypeLd().getValue();
        // Unboxing a live data value directly may
        // produce a null pointer exception, so it's required
        // checking against null.
        if (coinBoxed == null) {
            return;
        }
        @CoinType.EnumType int coin = coinBoxed;
        mJsonRpcService.getNetwork(
                coin,
                null,
                networkInfo -> {
                    if (networkInfo != null) {
                        mMediatorChainId.postValue(networkInfo.chainId);
                    }
                });
    }
}

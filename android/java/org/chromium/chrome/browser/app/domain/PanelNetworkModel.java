/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.build.annotations.NullMarked;

import java.util.Objects;

// Network state for the dApp panel, resolved for the active origin.
@NullMarked
public class PanelNetworkModel extends NetworkModel {
    private @Nullable String mOriginSpec;

    public PanelNetworkModel(
            BraveWalletService braveWalletService,
            @NonNull JsonRpcService jsonRpcService,
            CryptoSharedData sharedData,
            CryptoSharedActions cryptoSharedActions) {
        super(braveWalletService, jsonRpcService, sharedData, cryptoSharedActions);
    }

    public void updateOriginSpec(@Nullable String originSpec) {
        if (Objects.equals(mOriginSpec, originSpec)) return;
        mOriginSpec = originSpec;
        updateChainId();
    }

    @Override
    void updateChainId() {
        if (mBraveWalletService == null) {
            return;
        }
        mBraveWalletService.getNetworkForSelectedAccountOnActiveOrigin(
                networkInfo -> {
                    if (networkInfo != null) {
                        mMediatorChainId.postValue(networkInfo.chainId);
                    }
                });
    }

    public void setNetworkWithAccountCheck(
            NetworkInfo networkToBeSetAsSelected,
            boolean setNetworkAsDefault,
            Callback1<Boolean> callback) {
        @Nullable NetworkInfo selectedNetwork = mDefaultNetwork.getValue();
        if (isSameNetwork(networkToBeSetAsSelected, selectedNetwork)) return;

        mBraveWalletService.ensureSelectedAccountForChain(
                networkToBeSetAsSelected.coin,
                networkToBeSetAsSelected.chainId,
                accountId -> {
                    if (accountId == null) {
                        mMediatorNeedToCreateAccountForNetwork.postValue(networkToBeSetAsSelected);
                        callback.call(false);
                        return;
                    }
                    if (setNetworkAsDefault) {
                        setDefaultNetwork(networkToBeSetAsSelected, callback);
                    } else {
                        setNetworkForSelectedAccountOnActiveOrigin(
                                networkToBeSetAsSelected, callback);
                    }
                });
    }

    public void setNetworkForSelectedAccountOnActiveOrigin(
            NetworkInfo networkToBeSetAsSelected, Callback1<Boolean> callback) {
        mBraveWalletService.setNetworkForSelectedAccountOnActiveOrigin(
                networkToBeSetAsSelected.chainId,
                success -> {
                    callback.call(success);
                    mCryptoActions.updateCoinType();
                    init();
                });
    }

    private boolean isSameNetwork(
            NetworkInfo networkToBeSetAsSelected, @Nullable NetworkInfo selectedNetwork) {
        return selectedNetwork != null
                && selectedNetwork.chainId.equals(networkToBeSetAsSelected.chainId)
                && selectedNetwork.coin == networkToBeSetAsSelected.coin;
    }
}

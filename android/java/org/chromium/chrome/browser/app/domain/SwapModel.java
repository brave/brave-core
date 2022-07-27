/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.brave_wallet.mojom.TxService;

public class SwapModel {
    private TxService mTxService;
    private KeyringService mKeyringService;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private AssetRatioService mAssetRatioService;
    private SwapService mSwapService;
    private BlockchainToken mTxToken;
    private MutableLiveData<Boolean> _mIsSwapEnabled;

    public SwapModel(TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, AssetRatioService mAssetRatioService,
            SwapService mSwapService, BlockchainToken mTxToken) {
        this.mTxService = mTxService;
        this.mKeyringService = mKeyringService;
        this.mBlockchainRegistry = mBlockchainRegistry;
        this.mJsonRpcService = mJsonRpcService;
        this.mEthTxManagerProxy = mEthTxManagerProxy;
        this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
        this.mBraveWalletService = mBraveWalletService;
        this.mAssetRatioService = mAssetRatioService;
        this.mSwapService = mSwapService;
        this.mTxToken = mTxToken;
    }
}

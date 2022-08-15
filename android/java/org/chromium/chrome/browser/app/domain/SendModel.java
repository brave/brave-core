/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.mojo.bindings.Callbacks;

public class SendModel {
    private TxService mTxService;
    private KeyringService mKeyringService;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private BlockchainToken mTxToken;

    public SendModel(TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, BlockchainToken mTxToken) {
        this.mTxService = mTxService;
        this.mKeyringService = mKeyringService;
        this.mBlockchainRegistry = mBlockchainRegistry;
        this.mJsonRpcService = mJsonRpcService;
        this.mEthTxManagerProxy = mEthTxManagerProxy;
        this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
        this.mBraveWalletService = mBraveWalletService;
        this.mTxToken = mTxToken;
        // if mTxToken is null, don't do anything
    }

    public void resetServices(TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService) {
        this.mTxService = mTxService;
        this.mKeyringService = mKeyringService;
        this.mBlockchainRegistry = mBlockchainRegistry;
        this.mJsonRpcService = mJsonRpcService;
        this.mEthTxManagerProxy = mEthTxManagerProxy;
        this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
        this.mBraveWalletService = mBraveWalletService;
    }

    public void setTxToken(BlockchainToken mTxToken) {
        this.mTxToken = mTxToken;
        // create and update balance and other corresponding details
    }

    public void sendSolanaToken(BlockchainToken token, String fromAddress, String toAddress,
            long lamportsValue, Callbacks.Callback3<Boolean, String, String> callback) {
        if (token == null) return;

        if (token.coin == CoinType.SOL && !TextUtils.isEmpty(token.contractAddress)) {
            // process SPL (Solana Program Library) tokens
            mSolanaTxManagerProxy.makeTokenProgramTransferTxData(token.contractAddress, fromAddress,
                    toAddress, lamportsValue, (solanaTxData, error, errorMessageMakeData) -> {
                        if (error == 0) {
                            mTxService.addUnapprovedTransaction(
                                    WalletUtils.toTxDataUnion(solanaTxData), fromAddress, null,
                                    null, (success, txMetaId, errorMessage) -> {
                                        callback.call(success, txMetaId, errorMessage);
                                    });
                        }
                    });
        } else {
            // process native Solana (SOL) asset
            mSolanaTxManagerProxy.makeSystemProgramTransferTxData(fromAddress, toAddress,
                    lamportsValue, (solanaTxData, error, errorMessageMakeData) -> {
                        if (error == 0) {
                            mTxService.addUnapprovedTransaction(
                                    WalletUtils.toTxDataUnion(solanaTxData), fromAddress, null,
                                    null, (success, txMetaId, errorMessage) -> {
                                        callback.call(success, txMetaId, errorMessage);
                                    });
                        }
                    });
        }
    }
}

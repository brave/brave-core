/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.FilTxData;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.mojo.bindings.Callbacks;

import java.text.ParseException;

public class SendModel {
    private static final String TAG = "SendModel";

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

    public void sendSolanaToken(String chainId, BlockchainToken token, String fromAddress,
            String toAddress, long lamportsValue,
            Callbacks.Callback3<Boolean, String, String> callback) {
        if (token == null) return;

        if (token.coin == CoinType.SOL && !TextUtils.isEmpty(token.contractAddress)) {
            // process SPL (Solana Program Library) tokens
            mSolanaTxManagerProxy.makeTokenProgramTransferTxData(chainId, token.contractAddress,
                    fromAddress, toAddress, lamportsValue,
                    (solanaTxData, error, errorMessageMakeData) -> {
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

    public void sendFilecoinToken(@NonNull final String chainId,
            @Nullable final BlockchainToken token, @NonNull final String fromAddress,
            @NonNull final String toAddress, @NonNull final String amount,
            @NonNull final Callbacks.Callback3<Boolean, String, String> callback) {
        if (token == null) {
            Log.e(TAG, "Token cannot be null.");
            return;
        }
        if (TextUtils.isEmpty(token.contractAddress)) {
            Log.e(TAG, "Contract address cannot be null or empty.");
            return;
        }
        if (TextUtils.isEmpty(amount)) {
            Log.e(TAG, "Amount to send cannot be null or empty.");
            return;
        }

        if (token.coin != CoinType.FIL) {
            throw new IllegalStateException();
        }

        final FilTxData filTxData = new FilTxData();
        filTxData.to = toAddress;
        filTxData.from = fromAddress;
        try {
            filTxData.value = Utils.multiplyByDecimals(amount, token.decimals).toString();
        } catch (ParseException parseException) {
            Log.e(TAG, "Error while parsing Filecoin amount to send.", parseException);
            return;
        }
        final TxDataUnion txDataUnion = new TxDataUnion();
        txDataUnion.setFilTxData(filTxData);

        mTxService.addUnapprovedTransaction(
                txDataUnion, fromAddress, null, null, (success, txMetaId, errorMessage) -> {
                    callback.call(success, txMetaId, errorMessage);
                });
    }
}

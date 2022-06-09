/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.DecryptRequest;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.GetEncryptionPublicKeyRequest;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.mojo.bindings.Callbacks.Callback1;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.List;

public class CryptoModel {
    private TxService mTxService;
    private PendingTxHelper mPendingTxHelper;
    private KeyringService mKeyringService;
    private LiveData<List<TransactionInfo>> mPendingTransactions;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private AssetRatioService mAssetRatioService;
    private CryptoSharedData mSharedData;
    private final MutableLiveData<Integer> _mCoinTypeMutableLiveData =
            new MutableLiveData<>(CoinType.ETH);
    public final LiveData<Integer> mCoinTypeMutableLiveData = _mCoinTypeMutableLiveData;
    private final MutableLiveData<BraveWalletDAppsActivity.ActivityType> _mProcessNextDAppsRequest =
            new MutableLiveData<>();
    public final LiveData<BraveWalletDAppsActivity.ActivityType> mProcessNextDAppsRequest =
            _mProcessNextDAppsRequest;

    private NetworkModel mNetworkModel;
    // Todo: create a models for portfolio
    // Todo: create method to create and return new models for Asset, Account,
    //  TransactionConfirmation

    public CryptoModel(TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, AssetRatioService mAssetRatioService) {
        this.mTxService = mTxService;
        this.mKeyringService = mKeyringService;
        this.mBlockchainRegistry = mBlockchainRegistry;
        this.mJsonRpcService = mJsonRpcService;
        this.mEthTxManagerProxy = mEthTxManagerProxy;
        this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
        this.mBraveWalletService = mBraveWalletService;
        this.mAssetRatioService = mAssetRatioService;
        mSharedData = new CryptoSharedDataImpl();
        mPendingTxHelper = new PendingTxHelper(mTxService, new AccountInfo[0], true, true);
        mNetworkModel = new NetworkModel(mJsonRpcService, mSharedData);
    }

    public void resetServices(TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, AssetRatioService mAssetRatioService) {
        this.mTxService = mTxService;
        this.mKeyringService = mKeyringService;
        this.mBlockchainRegistry = mBlockchainRegistry;
        this.mJsonRpcService = mJsonRpcService;
        this.mEthTxManagerProxy = mEthTxManagerProxy;
        this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
        this.mBraveWalletService = mBraveWalletService;
        this.mAssetRatioService = mAssetRatioService;
        mPendingTxHelper.setTxService(mTxService);
        mNetworkModel.resetServices(mJsonRpcService);
        init();
    }

    public void init() {
        getPendingTxHelper().fetchTransactions(null);
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID,
                keyringInfo -> { mPendingTxHelper.setAccountInfos(keyringInfo.accountInfos); });

        // filter out a separate list of unapproved transactions
        mPendingTransactions =
                Transformations.map(mPendingTxHelper.mTransactionInfoLd, transactionInfos -> {
                    List<TransactionInfo> pendingTransactionInfo = new ArrayList<>();
                    for (TransactionInfo info : transactionInfos) {
                        if (info.txStatus == TransactionStatus.UNAPPROVED) {
                            pendingTransactionInfo.add(info);
                        }
                    }
                    return pendingTransactionInfo;
                });
        mNetworkModel.init();
    }

    public void getPublicEncryptionRequest(Callback1<GetEncryptionPublicKeyRequest> onResult) {
        mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
            GetEncryptionPublicKeyRequest request = null;
            if (requests != null && requests.length > 0) {
                request = requests[0];
            }
            onResult.call(request);
        });
    }

    public void getDecryptMessageRequest(Callback1<DecryptRequest> onResult) {
        mBraveWalletService.getPendingDecryptRequests(requests -> {
            DecryptRequest request = null;
            if (requests != null && requests.length > 0) {
                request = requests[0];
            }
            onResult.call(request);
        });
    }

    public void refreshTransactions() {
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID,
                keyringInfo -> { mPendingTxHelper.setAccountInfos(keyringInfo.accountInfos); });
    }

    public LiveData<TransactionInfo> getSelectedPendingRequest() {
        return mPendingTxHelper.mSelectedPendingRequest;
    }

    public LiveData<List<TransactionInfo>> getPendingTransactions() {
        return mPendingTransactions;
    }

    public LiveData<List<TransactionInfo>> getAllTransactions() {
        return mPendingTxHelper.mTransactionInfoLd;
    }

    public void processPublicEncryptionKey(boolean isApproved, Origin origin) {
        mBraveWalletService.notifyGetPublicKeyRequestProcessed(isApproved, origin);
        mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
            if (requests != null && requests.length > 0) {
                _mProcessNextDAppsRequest.postValue(
                        BraveWalletDAppsActivity.ActivityType.GET_ENCRYPTION_PUBLIC_KEY_REQUEST);
            } else {
                _mProcessNextDAppsRequest.postValue(BraveWalletDAppsActivity.ActivityType.FINISH);
            }
        });
    }

    public void clearDappsState() {
        _mProcessNextDAppsRequest.postValue(null);
    }

    public void processDecryptRequest(boolean isApproved, Origin origin) {
        mBraveWalletService.notifyDecryptRequestProcessed(isApproved, origin);
        mBraveWalletService.getPendingDecryptRequests(requests -> {
            if (requests != null && requests.length > 0) {
                _mProcessNextDAppsRequest.postValue(
                        BraveWalletDAppsActivity.ActivityType.DECRYPT_REQUEST);
            } else {
                _mProcessNextDAppsRequest.postValue(BraveWalletDAppsActivity.ActivityType.FINISH);
            }
        });
    }

    public PendingTxHelper getPendingTxHelper() {
        return mPendingTxHelper;
    }

    public CryptoSharedData getSharedData() {
        return mSharedData;
    }

    public NetworkModel getNetworkModel() {
        return mNetworkModel;
    }

    /*
     * A container class to share the required data throughout the domain model classes.
     * Note: It should only be used/accessed within the domain package
     */
    class CryptoSharedDataImpl implements CryptoSharedData {
        @Override
        public int getCoinType() {
            if (mCoinTypeMutableLiveData.getValue() == null) {
                return CoinType.ETH;
            }
            return mCoinTypeMutableLiveData.getValue();
        }

        @Override
        public String getChainId() {
            if (mNetworkModel.mChainId.getValue() == null) {
                return BraveWalletConstants.MAINNET_CHAIN_ID;
            }
            return mNetworkModel.mChainId.getValue();
        }
    }
}

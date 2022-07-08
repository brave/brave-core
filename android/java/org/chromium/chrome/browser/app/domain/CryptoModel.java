/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;

import com.google.android.gms.common.util.ArrayUtils;

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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.mojo.bindings.Callbacks.Callback1;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
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
    private final Object mLock = new Object();
    private Context mContext;

    private NetworkModel mNetworkModel;
    // Todo: create a models for portfolio
    // Todo: create method to create and return new models for Asset, Account,
    //  TransactionConfirmation

    public CryptoModel(Context context, TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, AssetRatioService mAssetRatioService) {
        mContext = context;
        this.mTxService = mTxService;
        this.mKeyringService = mKeyringService;
        this.mBlockchainRegistry = mBlockchainRegistry;
        this.mJsonRpcService = mJsonRpcService;
        this.mEthTxManagerProxy = mEthTxManagerProxy;
        this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
        this.mBraveWalletService = mBraveWalletService;
        this.mAssetRatioService = mAssetRatioService;
        mSharedData = new CryptoSharedDataImpl();
        mPendingTxHelper = new PendingTxHelper(mTxService, new AccountInfo[0], true, null, true);
        mNetworkModel = new NetworkModel(mJsonRpcService, mSharedData);
    }

    public void resetServices(Context context, TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, AssetRatioService mAssetRatioService) {
        synchronized (mLock) {
            mContext = context;
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
        }
        init();
    }

    public void init() {
        synchronized (mLock) {
            if (mKeyringService == null || mPendingTxHelper == null) {
                return;
            }
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
    }

    public void getPublicEncryptionRequest(Callback1<GetEncryptionPublicKeyRequest> onResult) {
        synchronized (mLock) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
                GetEncryptionPublicKeyRequest request = null;
                if (requests != null && requests.length > 0) {
                    request = requests[0];
                }
                onResult.call(request);
            });
        }
    }

    public void getDecryptMessageRequest(Callback1<DecryptRequest> onResult) {
        synchronized (mLock) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.getPendingDecryptRequests(requests -> {
                DecryptRequest request = null;
                if (requests != null && requests.length > 0) {
                    request = requests[0];
                }
                onResult.call(request);
            });
        }
    }

    public void refreshTransactions() {
        synchronized (mLock) {
            if (mKeyringService == null || mPendingTxHelper == null) {
                return;
            }
            mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID,
                    keyringInfo -> { mPendingTxHelper.setAccountInfos(keyringInfo.accountInfos); });
        }
    }

    public LiveData<TransactionInfo> getSelectedPendingRequest() {
        synchronized (mLock) {
            if (mPendingTxHelper == null) {
                return new MutableLiveData<>();
            }
            return mPendingTxHelper.mSelectedPendingRequest;
        }
    }

    public LiveData<List<TransactionInfo>> getPendingTransactions() {
        return mPendingTransactions;
    }

    public LiveData<List<TransactionInfo>> getAllTransactions() {
        synchronized (mLock) {
            if (mPendingTxHelper == null) {
                return new MutableLiveData<>(Collections.emptyList());
            }
            return mPendingTxHelper.mTransactionInfoLd;
        }
    }

    public void processPublicEncryptionKey(boolean isApproved, Origin origin) {
        synchronized (mLock) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.notifyGetPublicKeyRequestProcessed(isApproved, origin);
            mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
                if (requests != null && requests.length > 0) {
                    _mProcessNextDAppsRequest.postValue(BraveWalletDAppsActivity.ActivityType
                                                                .GET_ENCRYPTION_PUBLIC_KEY_REQUEST);
                } else {
                    _mProcessNextDAppsRequest.postValue(
                            BraveWalletDAppsActivity.ActivityType.FINISH);
                }
            });
        }
    }

    public void clearDappsState() {
        _mProcessNextDAppsRequest.postValue(null);
    }

    public void processDecryptRequest(boolean isApproved, Origin origin) {
        synchronized (mLock) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.notifyDecryptRequestProcessed(isApproved, origin);
            mBraveWalletService.getPendingDecryptRequests(requests -> {
                if (requests != null && requests.length > 0) {
                    _mProcessNextDAppsRequest.postValue(
                            BraveWalletDAppsActivity.ActivityType.DECRYPT_REQUEST);
                } else {
                    _mProcessNextDAppsRequest.postValue(
                            BraveWalletDAppsActivity.ActivityType.FINISH);
                }
            });
        }
    }

    public List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes() {
        List<CryptoAccountTypeInfo> cryptoAccountTypeInfos = new ArrayList<>();
        cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                mContext.getString(R.string.brave_wallet_create_account_ethereum_description),
                "Ethereum", CoinType.ETH, R.drawable.eth));
        if (isSolanaEnabled()) {
            cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                    mContext.getString(R.string.brave_wallet_create_account_solana_description),
                    "Solana", CoinType.SOL, R.drawable.ic_sol_asset_icon));
        }
        return cryptoAccountTypeInfos;
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

    public boolean isSolanaEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_WALLET_SOLANA);
    }

    public void updateCoinType() {
        mBraveWalletService.getSelectedCoin(
                coinType -> { _mCoinTypeMutableLiveData.postValue(coinType); });
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

        @Override
        public Context getContext() {
            return mContext;
        }

        @Override
        public LiveData<Integer> getCoinTypeLd() {
            return mCoinTypeMutableLiveData;
        }

        @Override
        public String[] getEnabledKeyrings() {
            ArrayList<String> keyRings = new ArrayList<>();
            keyRings.add(BraveWalletConstants.DEFAULT_KEYRING_ID);
            if (isSolanaEnabled()) {
                keyRings.add(BraveWalletConstants.SOLANA_KEYRING_ID);
            }
            return keyRings.toArray(new String[0]);
        }
    }
}

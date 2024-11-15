/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.base.Callbacks.Callback1;
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
import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// Unused members, never read:
// - mSolanaTxManagerProxy
// - mSwapService
// - mBlockchainRegistry
// - mEthTxManagerProxy
// - mAssetRatioService
@SuppressWarnings("UnusedVariable")
public class CryptoModel {
    private TxService mTxService;
    private final PendingTxHelper mPendingTxHelper;
    private KeyringService mKeyringService;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private AssetRatioService mAssetRatioService;
    private SwapService mSwapService;
    private CryptoSharedActions mCryptoSharedActions;
    private final CryptoSharedData mSharedData;
    private final MutableLiveData<Integer> _mCoinTypeMutableLiveData =
            new MutableLiveData<>(CoinType.ETH);
    private final LiveData<Integer> mCoinTypeMutableLiveData = _mCoinTypeMutableLiveData;

    private final Object mLock = new Object();
    private Context mContext;

    private final NetworkModel mNetworkModel;

    public LiveData<List<AccountInfo>> mAccountInfosFromKeyRingModel;

    public CryptoModel(Context context, TxService txService, KeyringService keyringService,
            BlockchainRegistry blockchainRegistry, JsonRpcService jsonRpcService,
            EthTxManagerProxy ethTxManagerProxy, SolanaTxManagerProxy solanaTxManagerProxy,
            BraveWalletService braveWalletService, AssetRatioService assetRatioService,
            CryptoSharedActions cryptoSharedActions, SwapService swapService) {
        mContext = context;
        mTxService = txService;
        mKeyringService = keyringService;
        mBlockchainRegistry = blockchainRegistry;
        mJsonRpcService = jsonRpcService;
        mEthTxManagerProxy = ethTxManagerProxy;
        mSolanaTxManagerProxy = solanaTxManagerProxy;
        mBraveWalletService = braveWalletService;
        mAssetRatioService = assetRatioService;
        mSwapService = swapService;
        mCryptoSharedActions = cryptoSharedActions;
        mSharedData = new CryptoSharedDataImpl();
        mPendingTxHelper = new PendingTxHelper(mTxService, new AccountInfo[0], true, true);
        mNetworkModel =
                new NetworkModel(
                        mBraveWalletService, mJsonRpcService, mSharedData, mCryptoSharedActions);
    }

    public void resetServices(
            Context context,
            TxService mTxService,
            KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry,
            JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy,
            SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService,
            AssetRatioService mAssetRatioService) {
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
            mNetworkModel.resetServices(mBraveWalletService, mJsonRpcService);
        }
        init();
    }

    public void init() {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            refreshTransactions();
            updateCoinType();
            mNetworkModel.init();
        }
    }

    public void getPublicEncryptionRequest(Callback1<GetEncryptionPublicKeyRequest> onResult) {
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

    public void getDecryptMessageRequest(Callback1<DecryptRequest> onResult) {
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

    public void refreshTransactions() {
        if (mKeyringService == null) {
            return;
        }

        mKeyringService.getAllAccounts(
                allAccounts -> mPendingTxHelper.setAccountInfos(allAccounts.accounts));
    }

    public LiveData<List<TransactionInfo>> getAllTransactions() {
        return mPendingTxHelper.mTransactionInfoLd;
    }

    public List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes() {
        List<CryptoAccountTypeInfo> cryptoAccountTypeInfos = new ArrayList<>();
        cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                mContext.getString(R.string.brave_wallet_create_account_ethereum_description),
                mContext.getString(R.string.wallet_eth_name), CoinType.ETH, R.drawable.eth));

        cryptoAccountTypeInfos.add(
                new CryptoAccountTypeInfo(
                        mContext.getString(R.string.brave_wallet_create_account_solana_description),
                        mContext.getString(R.string.wallet_sol_name),
                        CoinType.SOL,
                        R.drawable.ic_sol_asset_icon));

        cryptoAccountTypeInfos.add(
                new CryptoAccountTypeInfo(
                        mContext.getString(
                                R.string.brave_wallet_create_account_filecoin_description),
                        mContext.getString(R.string.wallet_fil_name),
                        CoinType.FIL,
                        R.drawable.ic_fil_asset_icon));
        return cryptoAccountTypeInfos;
    }

    @NonNull
    public PendingTxHelper getPendingTxHelper() {
        return mPendingTxHelper;
    }

    public CryptoSharedData getSharedData() {
        return mSharedData;
    }

    public NetworkModel getNetworkModel() {
        return mNetworkModel;
    }

    public void updateCoinType() {
        mKeyringService.getAllAccounts(
                allAccounts -> {
                    @CoinType.EnumType int coin = CoinType.ETH;

                    // null selectedAccount may happen in tests.
                    if (allAccounts.selectedAccount != null) {
                        // Current coin is the coin of selected account.
                        coin = allAccounts.selectedAccount.accountId.coin;
                    }

                    _mCoinTypeMutableLiveData.postValue(coin);
                });
    }

    /**
     * Initialise the account observable via setter (to avoid dependency cycle)
     *
     * @param accountInfosFromKeyRingModel from the keyrin model
     */
    public void setAccountInfosFromKeyRingModel(
            LiveData<List<AccountInfo>> accountInfosFromKeyRingModel) {
        this.mAccountInfosFromKeyRingModel = accountInfosFromKeyRingModel;
        // pass on the observer to other object
        mNetworkModel.setAccountInfosFromKeyRingModel(accountInfosFromKeyRingModel);
    }

    public void updateNftDiscovery(boolean isEnabled) {
        mBraveWalletService.setNftDiscoveryEnabled(isEnabled);
    }

    public void isNftDiscoveryEnabled(Callback1<Boolean> callback) {
        mBraveWalletService.getNftDiscoveryEnabled(
                isNftDiscoveryEnabled -> { callback.call(isNftDiscoveryEnabled); });
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
        public List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes() {
            return CryptoModel.this.getSupportedCryptoAccountTypes();
        }

        @Override
        public List<Integer> getSupportedCryptoCoins() {
            return Arrays.asList(CoinType.ETH, CoinType.SOL, CoinType.FIL, CoinType.BTC);
        }

        @Override
        public LiveData<List<AccountInfo>> getAccounts() {
            return mAccountInfosFromKeyRingModel;
        }
    }
}

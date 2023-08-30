/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.base.BraveFeatureList;
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
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.mojo.bindings.Callbacks.Callback1;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

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
    private CryptoSharedData mSharedData;
    private final MutableLiveData<Integer> _mCoinTypeMutableLiveData =
            new MutableLiveData<>(CoinType.ETH);
    public final LiveData<Integer> mCoinTypeMutableLiveData = _mCoinTypeMutableLiveData;

    private final Object mLock = new Object();
    private Context mContext;

    private NetworkModel mNetworkModel;
    private PortfolioModel mPortfolioModel;

    public LiveData<List<AccountInfo>> mAccountInfosFromKeyRingModel;
    private TransactionsModel mTransactionsModel;

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
        mPendingTxHelper = new PendingTxHelper(mTxService, new AccountInfo[0], true, true, null);
        mNetworkModel = new NetworkModel(
                mBraveWalletService, mJsonRpcService, mSharedData, mCryptoSharedActions, context);
        mPortfolioModel = new PortfolioModel(context, mTxService, mKeyringService,
                mBlockchainRegistry, mJsonRpcService, mEthTxManagerProxy, mSolanaTxManagerProxy,
                mBraveWalletService, mAssetRatioService, mSharedData);
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
            mNetworkModel.resetServices(mBraveWalletService, mJsonRpcService);
            mPortfolioModel.resetServices(context, mTxService, mKeyringService, mBlockchainRegistry,
                    mJsonRpcService, mEthTxManagerProxy, mSolanaTxManagerProxy, mBraveWalletService,
                    mAssetRatioService);
            if (mTransactionsModel != null) {
                mTransactionsModel.resetServices(mContext, mTxService, mKeyringService,
                        mBlockchainRegistry, mJsonRpcService, mEthTxManagerProxy,
                        mSolanaTxManagerProxy, mBraveWalletService, mAssetRatioService);
            }
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

    public LiveData<TransactionInfo> getSelectedPendingRequest() {
        return mPendingTxHelper.mSelectedPendingRequest;
    }

    public LiveData<List<TransactionInfo>> getPendingTransactions() {
        return mPendingTxHelper.mPendingTransactionInfoLd;
    }

    public LiveData<List<TransactionInfo>> getAllTransactions() {
        return mPendingTxHelper.mTransactionInfoLd;
    }

    public List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes() {
        List<CryptoAccountTypeInfo> cryptoAccountTypeInfos = new ArrayList<>();
        cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                mContext.getString(R.string.brave_wallet_create_account_ethereum_description),
                mContext.getString(R.string.wallet_eth_name), CoinType.ETH, R.drawable.eth));

        if (isSolanaEnabled()) {
            cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                    mContext.getString(R.string.brave_wallet_create_account_solana_description),
                    mContext.getString(R.string.wallet_sol_name), CoinType.SOL,
                    R.drawable.ic_sol_asset_icon));
        }
        if (isFilecoinEnabled()) {
            cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                    mContext.getString(R.string.brave_wallet_create_account_filecoin_description),
                    mContext.getString(R.string.wallet_fil_name), CoinType.FIL,
                    R.drawable.ic_fil_asset_icon));
        }
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

    public PortfolioModel getPortfolioModel() {
        return mPortfolioModel;
    }

    public TransactionsModel createTransactionModel() {
        if (mTransactionsModel == null) {
            mTransactionsModel = new TransactionsModel(mContext, mTxService, mKeyringService,
                    mBlockchainRegistry, mJsonRpcService, mEthTxManagerProxy, mSolanaTxManagerProxy,
                    mBraveWalletService, mAssetRatioService, mSharedData);
        }
        return mTransactionsModel;
    }

    public UserAssetModel createUserAssetModel(WalletCoinAdapter.AdapterType type) {
        return new UserAssetModel(
                mBraveWalletService, mJsonRpcService, mBlockchainRegistry, mSharedData, type);
    }

    public boolean isSolanaEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_WALLET_SOLANA);
    }

    public boolean isFilecoinEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_WALLET_FILECOIN);
    }

    public void updateCoinType() {
        updateCoinType(null, null);
    }

    public void updateCoinType(Integer coin, Callback1<Integer> callback) {
        if (coin == null) {
            updateCoinType(null);
        } else {
            _mCoinTypeMutableLiveData.postValue(coin);
            if (callback != null) {
                callback.call(coin);
            }
        }
    }

    public void updateCoinType(Callback1<Integer> callback) {
        mKeyringService.getAllAccounts(allAccounts -> {
            @CoinType.EnumType
            int coin = CoinType.ETH;

            // null selectedAccount may happen in tests.
            if (allAccounts.selectedAccount != null) {
                // Current coin is the coin of selected account.
                coin = allAccounts.selectedAccount.accountId.coin;
            }

            _mCoinTypeMutableLiveData.postValue(coin);
            if (callback != null) {
                callback.call(coin);
            }
        });
    }

    /**
     * Initialise the account observable via setter (to avoid dependency cycle)
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
            return getSupportedCryptoAccountTypes()
                    .stream()
                    .map(CryptoAccountTypeInfo::getCoinType)
                    .collect(Collectors.toList());
        }

        @Override
        public LiveData<List<AccountInfo>> getAccounts() {
            return mAccountInfosFromKeyRingModel;
        }
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;
import androidx.lifecycle.Transformations;

import org.chromium.base.BraveFeatureList;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.DecryptRequest;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.GetEncryptionPublicKeyRequest;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.JsonRpcServiceObserver;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SelectedAccountResponsesCollector;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.mojo.bindings.Callbacks.Callback1;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class CryptoModel {
    private TxService mTxService;
    private PendingTxHelper mPendingTxHelper;
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
    // SolanaProvider for solana dapps
    private LiveData<List<TransactionInfo>> mPendingTransactions;
    private MediatorLiveData<Boolean> _mIsSwapEnabled;
    private final MutableLiveData<Integer> _mCoinTypeMutableLiveData =
            new MutableLiveData<>(CoinType.ETH);
    public final LiveData<Integer> mCoinTypeMutableLiveData = _mCoinTypeMutableLiveData;

    private final Object mLock = new Object();
    private Context mContext;
    private SendModel mSendModel;

    private NetworkModel mNetworkModel;
    private PortfolioModel mPortfolioModel;
    // Todo: create method to create and return new models for Asset, Account,
    //  TransactionConfirmation, SwapModel, AssetModel, SendModel

    public LiveData<List<AccountInfo>> mAccountInfosFromKeyRingModel;
    public LiveData<Boolean> mIsSwapEnabled;

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
                new NetworkModel(mJsonRpcService, mSharedData, mCryptoSharedActions, context);
        mPortfolioModel = new PortfolioModel(context, mTxService, mKeyringService,
                mBlockchainRegistry, mJsonRpcService, mEthTxManagerProxy, mSolanaTxManagerProxy,
                mBraveWalletService, mAssetRatioService, mSharedData);
        _mIsSwapEnabled = new MediatorLiveData<>();
        mIsSwapEnabled = _mIsSwapEnabled;
        _mIsSwapEnabled.addSource(mNetworkModel.mChainId, chainId -> {
            mSwapService.isSwapSupported(
                    chainId, isSwapSupported -> { _mIsSwapEnabled.postValue(isSwapSupported); });
        });
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
            mPortfolioModel.resetServices(context, mTxService, mKeyringService, mBlockchainRegistry,
                    mJsonRpcService, mEthTxManagerProxy, mSolanaTxManagerProxy, mBraveWalletService,
                    mAssetRatioService);
        }
        init();
    }

    public void init() {
        synchronized (mLock) {
            if (mKeyringService == null || mPendingTxHelper == null) {
                return;
            }
            // TODO(pav): uncomment the below and move to refreshTransactions.
            // To process pending tx from all networks and full network name

            //            mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(),
            //            keyringInfos -> {
            //                List<AccountInfo> accountInfos =
            //                        WalletUtils.getAccountInfosFromKeyrings(keyringInfos);
            //                mPendingTxHelper.setAccountInfos(accountInfos);
            //            });

            refreshTransactions();

            // Filter out a separate list of unapproved transactions
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
            mBraveWalletService.getSelectedCoin(
                    coinType -> { _mCoinTypeMutableLiveData.postValue(coinType); });
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
            List<Integer> coins = new ArrayList<>();
            for (CryptoAccountTypeInfo cryptoAccountTypeInfo :
                    mSharedData.getSupportedCryptoAccountTypes()) {
                coins.add(cryptoAccountTypeInfo.getCoinType());
            }

            mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(), keyringInfos -> {
                List<AccountInfo> accountInfos =
                        WalletUtils.getAccountInfosFromKeyrings(keyringInfos);
                new SelectedAccountResponsesCollector(mKeyringService, coins, accountInfos)
                        .getAccounts(defaultAccountPerCoin -> {
                            mPendingTxHelper.setAccountInfos(
                                    new ArrayList<>(defaultAccountPerCoin));
                        });
            });
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
        return mPendingTxHelper.mPendingTransactionInfoLd;
    }

    public LiveData<List<TransactionInfo>> getAllTransactions() {
        synchronized (mLock) {
            if (mPendingTxHelper == null) {
                return new MutableLiveData<>(Collections.emptyList());
            }
            return mPendingTxHelper.mTransactionInfoLd;
        }
    }

    public List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes() {
        List<CryptoAccountTypeInfo> cryptoAccountTypeInfos = new ArrayList<>();
        if (isSolanaEnabled()) {
            cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                    mContext.getString(R.string.brave_wallet_create_account_solana_description),
                    mContext.getString(R.string.wallet_sol_name), CoinType.SOL,
                    R.drawable.ic_sol_asset_icon));
        }
        cryptoAccountTypeInfos.add(new CryptoAccountTypeInfo(
                mContext.getString(R.string.brave_wallet_create_account_ethereum_description),
                mContext.getString(R.string.wallet_eth_name), CoinType.ETH, R.drawable.eth));
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

    public PortfolioModel getPortfolioModel() {
        return mPortfolioModel;
    }

    public SendModel createSendModel() {
        if (mSendModel != null) return mSendModel;
        mSendModel =
                new SendModel(mTxService, mKeyringService, mBlockchainRegistry, mJsonRpcService,
                        mEthTxManagerProxy, mSolanaTxManagerProxy, mBraveWalletService, null);
        return mSendModel;
    }

    public boolean isSolanaEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_WALLET_SOLANA);
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
        mBraveWalletService.getSelectedCoin(coinType -> {
            _mCoinTypeMutableLiveData.postValue(coinType);
            if (callback != null) {
                callback.call(coinType);
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

    // TODO: Move to BuyModel class
    public void isBuySupported(NetworkInfo selectedNetwork, String assetSymbol,
            String contractAddress, String chainId, Callback1<Boolean> callback) {
        TokenUtils.getBuyTokensFiltered(
                mBlockchainRegistry, selectedNetwork, TokenUtils.TokenType.ALL, tokens -> {
                    callback.call(JavaUtils.includes(tokens,
                            iToken
                            -> AssetUtils.Filters.isSameToken(
                                    iToken, assetSymbol, contractAddress, chainId)));
                });
    }

    // Clear buy send swap model
    public void clearBSS() {
        mSendModel = null;
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

        @Override
        public List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes() {
            return CryptoModel.this.getSupportedCryptoAccountTypes();
        }

        @Override
        public LiveData<List<AccountInfo>> getAccounts() {
            return mAccountInfosFromKeyRingModel;
        }
    }
}

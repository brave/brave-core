/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.TxServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SolanaTransactionsGasHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

public class TransactionsModel implements TxServiceObserverImpl.TxServiceObserverImplDelegate {
    private final CryptoSharedData mSharedData;
    private final Object mLock = new Object();
    private TxService mTxService;
    private KeyringService mKeyringService;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private AssetRatioService mAssetRatioService;
    private Context mContext;
    private final MutableLiveData<List<WalletListItemModel>> _mParsedTransactions;
    private final MutableLiveData<Boolean> _mIsLoading;
    private List<NetworkInfo> mAllNetworkInfoList;
    private WeakReference<BraveWalletBaseActivity> mActivityRef;
    public final LiveData<List<WalletListItemModel>> mParsedTransactions;
    public List<AccountInfo> mAllAccountInfoList;
    public LiveData<Boolean> mIsLoading;

    public TransactionsModel(Context context, TxService txService, KeyringService keyringService,
            BlockchainRegistry blockchainRegistry, JsonRpcService jsonRpcService,
            EthTxManagerProxy ethTxManagerProxy, SolanaTxManagerProxy solanaTxManagerProxy,
            BraveWalletService braveWalletService, AssetRatioService assetRatioService,
            CryptoSharedData sharedData) {
        mContext = context;
        mTxService = txService;
        mKeyringService = keyringService;
        mBlockchainRegistry = blockchainRegistry;
        mJsonRpcService = jsonRpcService;
        mEthTxManagerProxy = ethTxManagerProxy;
        mSolanaTxManagerProxy = solanaTxManagerProxy;
        mBraveWalletService = braveWalletService;
        mAssetRatioService = assetRatioService;
        mSharedData = sharedData;
        mAllNetworkInfoList = Collections.emptyList();
        _mParsedTransactions = new MutableLiveData<>();
        _mIsLoading = new MutableLiveData<>(false);
        mIsLoading = _mIsLoading;
        mParsedTransactions = _mParsedTransactions;
        mAllAccountInfoList = Collections.emptyList();
        addServiceObservers();
    }

    // Fetch all transactions of all accounts except rejected, and calculate balances, gas etc.
    public void update(WeakReference<BraveWalletBaseActivity> activityRef) {
        synchronized (mLock) {
            _mIsLoading.postValue(true);
            mActivityRef = activityRef;
            if (JavaUtils.anyNull(
                        mJsonRpcService, mKeyringService, mActivityRef, mActivityRef.get()))
                return;
            NetworkModel.getAllNetworks(mJsonRpcService, mSharedData.getSupportedCryptoCoins(), allNetworks -> {
                mAllNetworkInfoList = allNetworks;
                mKeyringService.getAllAccounts(allAccounts -> {
                    mAllAccountInfoList = Arrays.asList(allAccounts.accounts);
                    var allAccountsArray = allAccounts.accounts;
                    // Fetch transactions
                    PendingTxHelper pendingTxHelper =
                            new PendingTxHelper(mTxService, allAccountsArray, true, null);
                    pendingTxHelper.fetchTransactions(() -> {
                        HashMap<String, TransactionInfo[]> pendingTxInfos =
                                pendingTxHelper.getTransactions();
                        pendingTxHelper.destroy();
                        TransactionInfo[] filteredTransactions =
                                pendingTxInfos.values()
                                        .stream()
                                        .flatMap(
                                                transactionInfos -> Arrays.stream(transactionInfos))
                                        .filter(tx -> tx.txStatus != TransactionStatus.REJECTED)
                                        .toArray(TransactionInfo[] ::new);
                        if (filteredTransactions.length == 0) {
                            postTxListResponse(Collections.emptyList());
                            return;
                        }
                        // Fetch tokens, balances, price etc.
                        List<AssetAccountsNetworkBalance> assetAccountsNetworkBalances =
                                new ArrayList<>();
                        AtomicInteger balanceResultCounter = new AtomicInteger();
                        List<NetworkInfo> txNetworks =
                                mAllNetworkInfoList.stream()
                                        .filter(networkInfo
                                                -> Arrays.stream(filteredTransactions)
                                                           .anyMatch(transactionInfo
                                                                   -> transactionInfo.chainId.equals(
                                                                           networkInfo.chainId)))
                                        .collect(Collectors.toList());
                        for (NetworkInfo networkInfo : txNetworks) {
                            var accountInfoListPerCoin =
                                    mAllAccountInfoList.stream()
                                            .filter(accountInfo
                                                    -> accountInfo.accountId.coin
                                                            == networkInfo.coin)
                                            .collect(Collectors.toList());

                            Utils.getTxExtraInfo(mActivityRef, TokenUtils.TokenType.ALL,
                                    mAllNetworkInfoList, networkInfo,
                                    accountInfoListPerCoin.toArray(new AccountInfo[0]), null, false,
                                    (assetPrices, userAssetsList, nativeAssetsBalances,
                                            blockchainTokensBalances) -> {
                                        AssetAccountsNetworkBalance asset =
                                                new AssetAccountsNetworkBalance(assetPrices,
                                                        userAssetsList, nativeAssetsBalances,
                                                        blockchainTokensBalances, networkInfo,
                                                        accountInfoListPerCoin);
                                        assetAccountsNetworkBalances.add(asset);
                                        if (balanceResultCounter.incrementAndGet()
                                                == txNetworks.size()) {
                                            parseTransactions(mActivityRef,
                                                    assetAccountsNetworkBalances,
                                                    filteredTransactions, networkInfo.coin);
                                        }
                                    });
                        }
                    });
                });
            });
        }
    }

    private void parseTransactions(WeakReference<BraveWalletBaseActivity> activityRef,
            List<AssetAccountsNetworkBalance> assetAccountsNetworkBalances,
            TransactionInfo[] transactionInfoArr, int coin) {
        // Received balances of all network, can now fetch transaction
        var allAccountsArray = mAllAccountInfoList.toArray(new AccountInfo[0]);
        SolanaTransactionsGasHelper solanaTransactionsGasHelper =
                new SolanaTransactionsGasHelper(activityRef.get(), transactionInfoArr);
        solanaTransactionsGasHelper.maybeGetSolanaGasEstimations(() -> {
            List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
            var perTxSolanaFee = solanaTransactionsGasHelper.getPerTxFee();
            for (TransactionInfo txInfo : transactionInfoArr) {
                AccountInfo txAccountInfo =
                        Utils.findAccount(allAccountsArray, txInfo.fromAccountId);
                if (txAccountInfo == null) {
                    continue;
                }
                long solanaEstimatedTxFee = 0;
                if (perTxSolanaFee.get(txInfo.id) != null) {
                    solanaEstimatedTxFee = perTxSolanaFee.get(txInfo.id);
                }
                var txNetwork = NetworkUtils.findNetwork(mAllNetworkInfoList, txInfo.chainId, coin);
                var txExtraData =
                        assetAccountsNetworkBalances.stream()
                                .filter(data -> data.networkInfo.chainId.equals(txInfo.chainId))
                                .findFirst()
                                .get();
                ParsedTransaction parsedTx = ParsedTransaction.parseTransaction(txInfo, txNetwork,
                        allAccountsArray, txExtraData.assetPrices, solanaEstimatedTxFee,
                        txExtraData.userAssetsList, txExtraData.nativeAssetsBalances,
                        txExtraData.blockchainTokensBalances);
                WalletListItemModel itemModel = Utils.makeWalletItem(
                        activityRef.get(), txInfo, txNetwork, parsedTx, txAccountInfo);
                walletListItemModelList.add(itemModel);
            }
            postTxListResponse(walletListItemModelList);
        });
    }

    private void postTxListResponse(List<WalletListItemModel> walletListItemModelList) {
        _mParsedTransactions.postValue(walletListItemModelList);
        _mIsLoading.postValue(false);
    }

    private void addServiceObservers() {
        if (mTxService != null) {
            TxServiceObserverImpl walletServiceObserver = new TxServiceObserverImpl(this);
            mTxService.addObserver(walletServiceObserver);
        }
    }

    void resetServices(Context context, TxService mTxService, KeyringService mKeyringService,
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
            addServiceObservers();
        }
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        update(mActivityRef);
    }

    @Override
    public void onUnapprovedTxUpdated(TransactionInfo txInfo) {
        updateTx(txInfo, mActivityRef);
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        updateTx(txInfo, mActivityRef);
    }

    private void updateTx(
            TransactionInfo txInfo, WeakReference<BraveWalletBaseActivity> mActivityRef) {
        List<WalletListItemModel> items = _mParsedTransactions.getValue();
        if (items != null && txInfo.txStatus == TransactionStatus.REJECTED
                && items.stream().anyMatch(
                        walletItem -> walletItem.getTransactionInfo().id.equals(txInfo.id))) {
            // Remove rejected transaction
            items = items.stream()
                            .filter(walletItem
                                    -> !walletItem.getTransactionInfo().id.equals(txInfo.id))
                            .collect(Collectors.toList());
            _mParsedTransactions.postValue(items);
        } else if (items != null && txInfo.txStatus != TransactionStatus.UNAPPROVED) {
            // Update status
            WalletListItemModel walletItemTx =
                    items.stream()
                            .filter(walletItem
                                    -> walletItem.getTransactionInfo().id.equals(txInfo.id))
                            .findFirst()
                            .orElse(null);
            if (walletItemTx != null) {
                walletItemTx.getTransactionInfo().txStatus = txInfo.txStatus;
                Utils.updateWalletCoinTransactionStatus(
                        walletItemTx, mContext, walletItemTx.getTransactionInfo());
                _mParsedTransactions.postValue(items);
            }
        } else {
            update(mActivityRef);
        }
    }

    private static class AssetAccountsNetworkBalance {
        HashMap<String, Double> assetPrices;
        BlockchainToken[] userAssetsList;
        HashMap<String, Double> nativeAssetsBalances;
        HashMap<String, HashMap<String, Double>> blockchainTokensBalances;
        NetworkInfo networkInfo;
        List<AccountInfo> accountInfoList;

        public AssetAccountsNetworkBalance(HashMap<String, Double> assetPrices,
                BlockchainToken[] userAssetsList, HashMap<String, Double> nativeAssetsBalances,
                HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
                NetworkInfo networkInfo, List<AccountInfo> accountInfoList) {
            this.assetPrices = assetPrices;
            this.userAssetsList = userAssetsList;
            this.nativeAssetsBalances = nativeAssetsBalances;
            this.blockchainTokensBalances = blockchainTokensBalances;
            this.networkInfo = networkInfo;
            this.accountInfoList = accountInfoList;
        }
    }
}

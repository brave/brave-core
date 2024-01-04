/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import androidx.annotation.NonNull;

import org.chromium.base.Callbacks;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

public class PortfolioHelper {
    private static final String TAG = "PortfolioHelper";
    private final WeakReference<BraveWalletBaseActivity> mActivity;
    private List<NetworkInfo> mSelectedNetworks;
    private final AccountInfo[] mAccountInfos;

    // Data supplied as result
    private final List<BlockchainToken> mUserAssets; // aka selected assets
    private final List<BlockchainToken> mHiddenAssets;
    private Double mTotalFiatSum;
    // Always use Utils#tokenToString(BlockchainToken) to create key
    private HashMap<String, Double> mPerTokenFiatSum;
    private HashMap<String, Double> mPerTokenCryptoSum;
    private AssetTimePrice[] mFiatHistory;
    private int mFiatHistoryTimeframe;
    private final List<NetworkInfo> mCryptoNetworks;
    private Map<String, Integer> mAssertSortPriorityPerCoinIndex;

    public PortfolioHelper(BraveWalletBaseActivity activity, List<NetworkInfo> cryptoNetworks,
            AccountInfo[] accountInfos) {
        mActivity = new WeakReference<>(activity);
        mCryptoNetworks = cryptoNetworks;
        mAccountInfos = accountInfos;
        mSelectedNetworks = Collections.emptyList();
        mUserAssets = new ArrayList<>();
        mHiddenAssets = new ArrayList<>();
        mAssertSortPriorityPerCoinIndex = Collections.emptyMap();
    }

    public void setFiatHistoryTimeframe(int timeframe) {
        mFiatHistoryTimeframe = timeframe;
    }

    public List<BlockchainToken> getUserAssets() {
        return mUserAssets;
    }

    public List<BlockchainToken> getHiddenAssets() {
        return mHiddenAssets;
    }

    public Double getTotalFiatSum() {
        return mTotalFiatSum;
    }

    public HashMap<String, Double> getPerTokenFiatSum() {
        return mPerTokenFiatSum;
    }

    public HashMap<String, Double> getPerTokenCryptoSum() {
        return mPerTokenCryptoSum;
    }

    public AssetTimePrice[] getFiatHistory() {
        return mFiatHistory;
    }

    public boolean isFiatHistoryEmpty() {
        if (mFiatHistory == null || mFiatHistory.length == 0) {
            return true;
        }

        try {
            for (AssetTimePrice assetTimePrice : mFiatHistory) {
                if (Double.parseDouble(assetTimePrice.price) > 0.001d) {
                    return false;
                }
            }
        } catch (NullPointerException | NumberFormatException ignored) {
            return true;
        }
        return true;
    }

    public Double getMostRecentFiatSum() {
        if (mFiatHistory == null || mFiatHistory.length == 0) {
            return 0.0d;
        }
        return Double.valueOf(mFiatHistory[mFiatHistory.length - 1].price);
    }

    public void fetchAssetsAndDetails(
            TokenUtils.TokenType type, Callbacks.Callback1<PortfolioHelper> callback) {
        resetResultData();
        if (mActivity.get() == null || mSelectedNetworks.isEmpty()) return;
        int totalNetworks = mSelectedNetworks.size();
        AtomicInteger resCount = new AtomicInteger();
        List<AssetAccountsNetworkBalance> assetAccountsNetworkBalances = new ArrayList<>();

        for (NetworkInfo networkInfo : mSelectedNetworks) {
            List<AccountInfo> accountInfosPerCoin = JavaUtils.filter(
                    mAccountInfos, accountInfo -> accountInfo.accountId.coin == networkInfo.coin);
            Utils.getTxExtraInfo(mActivity, type, mCryptoNetworks, networkInfo,
                    accountInfosPerCoin.toArray(new AccountInfo[0]), null, true,
                    (assetPrices, userAssetsList, nativeAssetsBalances,
                            blockchainTokensBalances) -> {
                        mUserAssets.addAll(Arrays.asList(userAssetsList));

                        AssetAccountsNetworkBalance asset = new AssetAccountsNetworkBalance(
                                assetPrices, userAssetsList, nativeAssetsBalances,
                                blockchainTokensBalances, networkInfo, accountInfosPerCoin);
                        assetAccountsNetworkBalances.add(asset);

                        // Calculate balances only after the responses of network list are fetched
                        if (resCount.incrementAndGet() == totalNetworks) {
                            mUserAssets.sort(Comparator.comparing(
                                    token -> mAssertSortPriorityPerCoinIndex.get(token.chainId)));
                            createBalanceRecords(assetAccountsNetworkBalances);
                            callback.call(this);
                        }
                    });
        }
    }

    public void fetchNfts(@NonNull final Callbacks.Callback1<PortfolioHelper> callback) {
        resetResultData();
        if (mActivity.get() == null) {
            Log.e(TAG, "Referenced activity was null.");
            callback.call(this);
            return;
        }
        final BraveWalletBaseActivity activity = mActivity.get();
        if (activity.isFinishing()) {
            return;
        }
        if (mSelectedNetworks.isEmpty()) {
            Log.e(TAG, "Selected network was null.");
            callback.call(this);
            return;
        }
        final BraveWalletService braveWalletService = activity.getBraveWalletService();
        if (braveWalletService == null) {
            Log.e(TAG, "BraveWalletService was null.");
            callback.call(this);
            return;
        }
        final BlockchainRegistry blockchainRegistry = activity.getBlockchainRegistry();
        if (blockchainRegistry == null) {
            Log.e(TAG, "BlockchainRegistry was null.");
            callback.call(this);
            return;
        }
        final int totalNetworks = mSelectedNetworks.size();
        AtomicInteger count = new AtomicInteger();
        final List<BlockchainToken> assets = new ArrayList<>();
        for (NetworkInfo networkInfo : mSelectedNetworks) {
            TokenUtils.getUserAndAllTokensFiltered(braveWalletService, blockchainRegistry,
                    networkInfo, networkInfo.coin, TokenUtils.TokenType.NFTS,
                    (allAssets, userAssets) -> {
                        mUserAssets.addAll(Arrays.asList(userAssets));
                        assets.addAll(Arrays.asList(allAssets));
                        if (count.incrementAndGet() == totalNetworks) {
                            mUserAssets.sort(Comparator.comparing(
                                    token -> mAssertSortPriorityPerCoinIndex.get(token.chainId)));

                            List<String> comparableUserAssets =
                                    mUserAssets.stream()
                                            .map(Utils::tokenToString)
                                            .collect(Collectors.toList());
                            assets.removeIf(blockChainToken
                                    -> comparableUserAssets.contains(
                                            Utils.tokenToString(blockChainToken)));
                            mHiddenAssets.addAll(assets);

                            callback.call(this);
                        }
                    });
        }
    }

    private void createBalanceRecords(
            List<AssetAccountsNetworkBalance> assetAccountsNetworkBalances) {
        for (AssetAccountsNetworkBalance assetAccountsNetworkBalance :
                assetAccountsNetworkBalances) {
            // Sum across accounts
            for (AccountInfo accountInfo : assetAccountsNetworkBalance.accountInfos) {
                final String accountAddressLower =
                        accountInfo.address.toLowerCase(Locale.getDefault());
                HashMap<String, Double> thisAccountTokensBalances =
                        Utils.getOrDefault(assetAccountsNetworkBalance.blockchainTokensBalances,
                                accountAddressLower, new HashMap<String, Double>());
                for (BlockchainToken userAsset : assetAccountsNetworkBalance.userAssetsList) {
                    String currentAssetKey = Utils.tokenToString(userAsset);
                    double prevTokenCryptoBalanceSum =
                            Utils.getOrDefault(mPerTokenCryptoSum, currentAssetKey, 0.0d);
                    final double thisCryptoBalance =
                            Utils.isNativeToken(assetAccountsNetworkBalance.networkInfo, userAsset)
                            ? Utils.getOrDefault(assetAccountsNetworkBalance.nativeAssetsBalances,
                                    accountAddressLower, 0.0d)
                            : Utils.getOrDefault(thisAccountTokensBalances, currentAssetKey, 0.0d);

                    mPerTokenCryptoSum.put(
                            currentAssetKey, prevTokenCryptoBalanceSum + thisCryptoBalance);

                    double prevTokenFiatBalanceSum =
                            Utils.getOrDefault(mPerTokenFiatSum, currentAssetKey, 0.0d);
                    double thisTokenPrice =
                            Utils.getOrDefault(assetAccountsNetworkBalance.assetPrices,
                                    userAsset.symbol.toLowerCase(Locale.getDefault()), 0.0d);
                    double thisFiatBalance = thisTokenPrice * thisCryptoBalance;

                    mPerTokenFiatSum.put(
                            currentAssetKey, prevTokenFiatBalanceSum + thisFiatBalance);

                    mTotalFiatSum += thisFiatBalance;
                }
            }
        }
    }

    private void resetResultData() {
        mUserAssets.clear();
        mHiddenAssets.clear();
        mTotalFiatSum = 0.0d;
        mPerTokenFiatSum = new HashMap<>();
        mPerTokenCryptoSum = new HashMap<>();
        mFiatHistory = new AssetTimePrice[0];
    }

    private AssetTimePrice getZeroHistoryEntry(long microseconds) {
        AssetTimePrice historyEntry = new AssetTimePrice();
        historyEntry.price = "0";
        historyEntry.date = new TimeDelta();
        historyEntry.date.microseconds = microseconds;
        return historyEntry;
    }

    private AssetTimePrice[] getZeroPortfolioHistory() {
        // Gives two points of zero balance to make a chart
        AssetTimePrice[] history = new AssetTimePrice[2];
        history[0] = getZeroHistoryEntry((new java.util.Date()).getTime() * 1000);
        history[1] = getZeroHistoryEntry(history[0].date.microseconds - 1000 * 1000);
        return history;
    }

    public void calculateFiatHistory(Runnable runWhenDone) {
        mFiatHistory = new AssetTimePrice[0];
        var nonZeroBalanceAssetList =
                mUserAssets.stream()
                        .filter(token -> {
                            var assetBalance = mPerTokenCryptoSum.get(Utils.tokenToString(token));
                            return assetBalance != null && assetBalance > 0;
                        })
                        .collect(Collectors.toList());

        AsyncUtils.MultiResponseHandler historyMultiResponse =
                new AsyncUtils.MultiResponseHandler(nonZeroBalanceAssetList.size());

        ArrayList<AsyncUtils.GetPriceHistoryResponseContext> pricesHistoryContexts =
                new ArrayList<AsyncUtils.GetPriceHistoryResponseContext>();

        for (BlockchainToken userAsset : nonZeroBalanceAssetList) {
            AsyncUtils.GetPriceHistoryResponseContext priceHistoryContext =
                    new AsyncUtils.GetPriceHistoryResponseContext(
                            historyMultiResponse.singleResponseComplete);

            priceHistoryContext.userAsset = userAsset;
            pricesHistoryContexts.add(priceHistoryContext);

            if (mActivity.get() != null && !mActivity.get().isFinishing()) {
                mActivity.get().getAssetRatioService().getPriceHistory(
                        AssetUtils.assetRatioId(userAsset), "usd", mFiatHistoryTimeframe,
                        priceHistoryContext);
            }
        }

        historyMultiResponse.setWhenAllCompletedAction(() -> {
            // Algorithm is taken from the desktop:
            // components/brave_wallet_ui/common/reducers/wallet_reducer.ts
            // WalletActions.portfolioPriceHistoryUpdated:
            // 1. Exclude price history responses of zero length
            // 2. Choose the price history of the shortest length
            // 3. Per user selected token:
            //    3.1 multiply each token history-entry price by current tokens amount
            //    3.2 so have the history of per-date fiat balance per token
            // 4. Base on shortest history consolidate fiat history.
            //    4.1 take first date from shortest history prices, it may not
            //        the dates from the others tokens histories
            //    4.2 take first-entries from all tokens and sum them
            //    4.3 go through 4.1 and 4.2 till there are entries in shortest
            //        history. Some histories may have more entries - they are just ignored.

            Utils.removeIf(pricesHistoryContexts, phc -> phc.timePrices.length == 0);

            if (pricesHistoryContexts.isEmpty()) {
                // All history price requests failed
                mFiatHistory = getZeroPortfolioHistory();
                runWhenDone.run();
                return;
            }

            AsyncUtils.GetPriceHistoryResponseContext shortestPriceHistoryContext =
                    Collections.min(pricesHistoryContexts, (l, r) -> {
                        return Integer.compare(l.timePrices.length, r.timePrices.length);
                    });

            int shortestPricesLength = shortestPriceHistoryContext.timePrices.length;

            mFiatHistory = new AssetTimePrice[shortestPricesLength];

            // Go over the selected userAssets
            for (int i = 0; i < shortestPricesLength; ++i) {
                mFiatHistory[i] = new AssetTimePrice();
                mFiatHistory[i].date = shortestPriceHistoryContext.timePrices[i].date;

                Double thisDateFiatSum = 0.0d;
                for (AsyncUtils.GetPriceHistoryResponseContext priceHistoryContext :
                        pricesHistoryContexts) {
                    thisDateFiatSum += Float.parseFloat(priceHistoryContext.timePrices[i].price)
                            * Utils.getOrDefault(mPerTokenCryptoSum,
                                    Utils.tokenToString(priceHistoryContext.userAsset), 0.0d);
                }
                mFiatHistory[i].price = Double.toString(thisDateFiatSum);
            }

            runWhenDone.run();
        });
    }

    public void setSelectedNetworks(List<NetworkInfo> mSelectedNetworks) {
        this.mSelectedNetworks = mSelectedNetworks;
        updateAssetSortPriority(mSelectedNetworks);
    }

    // Update map of Map<ChainId, Priority/Network-Index>, to be used for sorting "All Networks"
    // asset list
    private void updateAssetSortPriority(List<NetworkInfo> networks) {
        mAssertSortPriorityPerCoinIndex = AssetUtils.toNetworkIndexMap(networks);
    }

    private static class AssetAccountsNetworkBalance {
        HashMap<String, Double> assetPrices;
        BlockchainToken[] userAssetsList;
        HashMap<String, Double> nativeAssetsBalances;
        HashMap<String, HashMap<String, Double>> blockchainTokensBalances;
        NetworkInfo networkInfo;
        List<AccountInfo> accountInfos;

        public AssetAccountsNetworkBalance(HashMap<String, Double> assetPrices,
                BlockchainToken[] userAssetsList, HashMap<String, Double> nativeAssetsBalances,
                HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
                NetworkInfo networkInfo, List<AccountInfo> accountInfos) {
            this.assetPrices = assetPrices;
            this.userAssetsList = userAssetsList;
            this.nativeAssetsBalances = nativeAssetsBalances;
            this.blockchainTokensBalances = blockchainTokensBalances;
            this.networkInfo = networkInfo;
            this.accountInfos = accountInfos;
        }
    }
}

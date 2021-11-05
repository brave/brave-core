/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.fromHexWei;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Locale;

public class PortfolioHelper {
    private static String TAG = "PortfolioHelper";
    private BraveWalletService mBraveWalletService;
    private AssetRatioController mAssetRatioController;
    private EthJsonRpcController mEthJsonRpcController;
    private String mChainId;
    private AccountInfo[] mAccountInfos;

    // Data supplied as result
    private ErcToken[] mUserAssets; // aka selected assets
    private Double mTotalFiatSum;
    private HashMap<String, Double> mPerTokenFiatSum;
    private HashMap<String, Double> mPerTokenCryptoSum;
    private AssetTimePrice[] mFiatHistory;
    private int mFiatHistoryTimeframe;

    public PortfolioHelper(BraveWalletService braveWalletService,
            AssetRatioController assetRatioController, EthJsonRpcController ethJsonRpcController,
            AccountInfo[] accountInfos) {
        assert braveWalletService != null;
        assert assetRatioController != null;
        assert ethJsonRpcController != null;
        mBraveWalletService = braveWalletService;
        mAssetRatioController = assetRatioController;
        mEthJsonRpcController = ethJsonRpcController;
        mAccountInfos = accountInfos;
    }

    public void setChainId(String chainId) {
        assert chainId != null && !chainId.isEmpty();
        mChainId = chainId;
    }

    public void setFiatHistoryTimeframe(int timeframe) {
        mFiatHistoryTimeframe = timeframe;
    }

    public ErcToken[] getUserAssets() {
        return mUserAssets;
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

        for (AssetTimePrice assetTimePrice : mFiatHistory) {
            if (Double.valueOf(assetTimePrice.price) > 0.001d) {
                return false;
            }
        }

        return true;
    }

    public Double getMostPreviousFiatSum() {
        if (mFiatHistory == null || mFiatHistory.length == 0) {
            return 0.0d;
        }
        return Double.valueOf(mFiatHistory[mFiatHistory.length - 1].price);
    }

    public void calculateBalances(Runnable runWhenDone) {
        resetResultData();

        mBraveWalletService.getUserAssets(mChainId, (userAssets) -> {
            mUserAssets = userAssets;
            HashMap<String, Double> tokenToUsdRatios = new HashMap<String, Double>();

            AsyncUtils.MultiResponseHandler pricesMultiResponse =
                    new AsyncUtils.MultiResponseHandler(mUserAssets.length);
            ArrayList<AsyncUtils.GetPriceResponseContext> pricesContexts =
                    new ArrayList<AsyncUtils.GetPriceResponseContext>();
            for (ErcToken userAsset : mUserAssets) {
                String[] fromAssets =
                        new String[] {userAsset.symbol.toLowerCase(Locale.getDefault())};
                String[] toAssets = new String[] {"usd"};

                AsyncUtils.GetPriceResponseContext priceContext =
                        new AsyncUtils.GetPriceResponseContext(
                                pricesMultiResponse.singleResponseComplete);

                pricesContexts.add(priceContext);

                mAssetRatioController.getPrice(
                        fromAssets, toAssets, AssetPriceTimeframe.LIVE, priceContext);
            }

            pricesMultiResponse.setWhenAllCompletedAction(() -> {
                for (AsyncUtils.GetPriceResponseContext priceContext : pricesContexts) {
                    if (!priceContext.success) {
                        continue;
                    }

                    assert priceContext.prices.length == 1;

                    Double usdPerToken = 0.0d;
                    try {
                        usdPerToken = Double.parseDouble(priceContext.prices[0].price);
                    } catch (NullPointerException | NumberFormatException ex) {
                        Log.e(TAG, "Cannot parse " + priceContext.prices[0].price + ", " + ex);
                        return;
                    }
                    tokenToUsdRatios.put(
                            priceContext.prices[0].fromAsset.toLowerCase(Locale.getDefault()),
                            usdPerToken);
                }

                AsyncUtils.MultiResponseHandler balancesMultiResponse =
                        new AsyncUtils.MultiResponseHandler(
                                mAccountInfos.length * mUserAssets.length);

                ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts =
                        new ArrayList<AsyncUtils.GetBalanceResponseBaseContext>();

                // Tokens balances
                for (AccountInfo accountInfo : mAccountInfos) {
                    for (ErcToken userAsset : mUserAssets) {
                        if (userAsset.contractAddress.isEmpty()) {
                            AsyncUtils.GetBalanceResponseContext context =
                                    new AsyncUtils.GetBalanceResponseContext(
                                            balancesMultiResponse.singleResponseComplete);
                            context.userAsset = userAsset;
                            contexts.add(context);
                            mEthJsonRpcController.getBalance(accountInfo.address, context);

                        } else {
                            AsyncUtils.GetErc20TokenBalanceResponseContext context =
                                    new AsyncUtils.GetErc20TokenBalanceResponseContext(
                                            balancesMultiResponse.singleResponseComplete);
                            context.userAsset = userAsset;
                            contexts.add(context);
                            mEthJsonRpcController.getErc20TokenBalance(
                                    userAsset.contractAddress, accountInfo.address, context);
                        }
                    }
                }

                balancesMultiResponse.setWhenAllCompletedAction(() -> {
                    for (AsyncUtils.GetBalanceResponseBaseContext context : contexts) {
                        String currentAssetSymbol =
                                context.userAsset.symbol.toLowerCase(Locale.getDefault());
                        Double usdPerThisToken =
                                Utils.getOrDefault(tokenToUsdRatios, currentAssetSymbol, 0.0d);

                        int decimals =
                                (context.userAsset.decimals != 0) ? context.userAsset.decimals : 18;

                        Double thisBalanceCryptoPart =
                                context.success ? fromHexWei(context.balance, decimals) : 0.0d;

                        Double thisBalanceFiatPart = usdPerThisToken * thisBalanceCryptoPart;

                        Double prevThisTokenCryptoSum =
                                Utils.getOrDefault(mPerTokenCryptoSum, currentAssetSymbol, 0.0d);

                        mPerTokenCryptoSum.put(
                                currentAssetSymbol, prevThisTokenCryptoSum + thisBalanceCryptoPart);

                        Double prevThisTokenFiatSum =
                                Utils.getOrDefault(mPerTokenFiatSum, currentAssetSymbol, 0.0d);
                        mPerTokenFiatSum.put(
                                currentAssetSymbol, prevThisTokenFiatSum + thisBalanceFiatPart);

                        mTotalFiatSum += thisBalanceFiatPart;
                    }

                    runWhenDone.run();
                });
            });
        });
    }

    private void resetResultData() {
        mUserAssets = new ErcToken[0];
        mTotalFiatSum = 0.0d;
        mPerTokenFiatSum = new HashMap<String, Double>();
        mPerTokenCryptoSum = new HashMap<String, Double>();
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

        AsyncUtils.MultiResponseHandler historyMultiResponse =
                new AsyncUtils.MultiResponseHandler(mUserAssets.length);

        ArrayList<AsyncUtils.GetPriceHistoryResponseContext> pricesHistoryContexts =
                new ArrayList<AsyncUtils.GetPriceHistoryResponseContext>();

        for (ErcToken userAsset : mUserAssets) {
            AsyncUtils.GetPriceHistoryResponseContext priceHistoryContext =
                    new AsyncUtils.GetPriceHistoryResponseContext(
                            historyMultiResponse.singleResponseComplete);

            priceHistoryContext.userAsset = userAsset;
            pricesHistoryContexts.add(priceHistoryContext);

            mAssetRatioController.getPriceHistory(userAsset.symbol.toLowerCase(Locale.getDefault()),
                    mFiatHistoryTimeframe, priceHistoryContext);
        }

        historyMultiResponse.setWhenAllCompletedAction(() -> {
            if (pricesHistoryContexts.isEmpty()) {
                // All history price requests failed
                mFiatHistory = getZeroPortfolioHistory();
                runWhenDone.run();
                return;
            }

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

            assert !pricesHistoryContexts.isEmpty();

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
                                    priceHistoryContext.userAsset.symbol.toLowerCase(
                                            Locale.getDefault()),
                                    0.0d);
                }
                mFiatHistory[i].price = Double.toString(thisDateFiatSum);
            }

            runWhenDone.run();
        });
    }
}

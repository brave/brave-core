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
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.ProviderError;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;

public class SingleTokenBalanceHelper {
    private static String TAG = "SingleAccountTokenBalance";
    private AssetRatioService mAssetRatioService;
    private JsonRpcService mJsonRpcService;
    private HashMap<String, Double> mPerAcountFiatBalance;
    private HashMap<String, Double> mPerAcountCryptoBalance;

    public SingleTokenBalanceHelper(
            AssetRatioService assetRatioService, JsonRpcService jsonRpcService) {
        mAssetRatioService = assetRatioService;
        mJsonRpcService = jsonRpcService;
    }

    public HashMap<String, Double> getPerAccountFiatBalance() {
        return mPerAcountFiatBalance;
    }

    public HashMap<String, Double> getPerAccountCryptoBalance() {
        return mPerAcountCryptoBalance;
    }

    public void getPerAccountBalances(String chainId, String contractAddress, String symbol,
            int decimals, AccountInfo[] accountInfos, Runnable runWhenDone) {
        mPerAcountFiatBalance = new HashMap<String, Double>();
        mPerAcountCryptoBalance = new HashMap<String, Double>();

        String[] fromAssets = new String[] {symbol.toLowerCase(Locale.getDefault())};
        String[] toAssets = new String[] {"usd"};

        mAssetRatioService.getPrice(fromAssets, toAssets, AssetPriceTimeframe.LIVE,
                (Boolean success, AssetPrice[] prices) -> {
                    // We have to do that to support custom assets
                    String price = "0";
                    if (success && prices.length != 0) {
                        price = prices[0].price;
                    }
                    Double usdPerToken;
                    try {
                        usdPerToken = Double.parseDouble(price);
                    } catch (NullPointerException | NumberFormatException ex) {
                        Log.e(TAG, "Cannot parse " + price + ", " + ex);
                        runWhenDone.run();
                        return;
                    }

                    AsyncUtils.MultiResponseHandler balancesMultiResponse =
                            new AsyncUtils.MultiResponseHandler(accountInfos.length);

                    ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts =
                            new ArrayList<AsyncUtils.GetBalanceResponseBaseContext>();

                    for (AccountInfo accountInfo : accountInfos) {
                        if (contractAddress.isEmpty()) {
                            AsyncUtils.GetBalanceResponseContext context =
                                    new AsyncUtils.GetBalanceResponseContext(
                                            balancesMultiResponse.singleResponseComplete);
                            context.accountAddress = accountInfo.address;
                            contexts.add(context);
                            mJsonRpcService.getBalance(
                                    accountInfo.address, CoinType.ETH, chainId, context);
                        } else {
                            AsyncUtils.GetErc20TokenBalanceResponseContext context =
                                    new AsyncUtils.GetErc20TokenBalanceResponseContext(
                                            balancesMultiResponse.singleResponseComplete);
                            context.accountAddress = accountInfo.address;
                            contexts.add(context);
                            mJsonRpcService.getErc20TokenBalance(
                                    Utils.getContractAddress(chainId, symbol, contractAddress),
                                    accountInfo.address, chainId, context);
                        }
                    }

                    balancesMultiResponse.setWhenAllCompletedAction(() -> {
                        final int decimalsNormalized = (decimals != 0) ? decimals : 18;

                        for (AsyncUtils.GetBalanceResponseBaseContext context : contexts) {
                            Double cryptoBalance = (context.error == ProviderError.SUCCESS)
                                    ? fromHexWei(context.balance, decimalsNormalized)
                                    : 0.0d;
                            mPerAcountCryptoBalance.put(context.accountAddress, cryptoBalance);
                            Double fiatBalance = usdPerToken * cryptoBalance;
                            mPerAcountFiatBalance.put(context.accountAddress, fiatBalance);
                        }

                        runWhenDone.run();
                    });
                });
    }
}

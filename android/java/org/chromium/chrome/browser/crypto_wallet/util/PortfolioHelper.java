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
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;

public class PortfolioHelper {
    private static String TAG = "PortfolioHelper";
    private BraveWalletService mBraveWalletService;
    private AssetRatioController mAssetRatioController;
    private KeyringController mKeyringController;
    private EthJsonRpcController mEthJsonRpcController;
    private String mChainId;

    // Data supplied as result
    private ErcToken[] mUserAssets; // aka selected assets
    private Double mTotalFiatSum;
    private HashMap<String, Double> mPerTokenFiatSum;
    private HashMap<String, Double> mPerTokenCryptoSum;

    public PortfolioHelper(BraveWalletService braveWalletService,
            AssetRatioController assetRatioController, KeyringController keyringController,
            EthJsonRpcController ethJsonRpcController) {
        assert braveWalletService != null;
        assert assetRatioController != null;
        assert keyringController != null;
        assert ethJsonRpcController != null;
        mBraveWalletService = braveWalletService;
        mAssetRatioController = assetRatioController;
        mKeyringController = keyringController;
        mEthJsonRpcController = ethJsonRpcController;
    }

    public void setChainId(String chainId) {
        assert chainId != null && !chainId.isEmpty();
        mChainId = chainId;
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

                mKeyringController.getDefaultKeyringInfo(keyringInfo -> {
                    if (keyringInfo != null) {
                        AccountInfo[] accountInfos = keyringInfo.accountInfos;

                        AsyncUtils.MultiResponseHandler balancesMultiResponse =
                                new AsyncUtils.MultiResponseHandler(
                                        accountInfos.length * mUserAssets.length);

                        ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts =
                                new ArrayList<AsyncUtils.GetBalanceResponseBaseContext>();

                        // Tokens balances
                        for (AccountInfo accountInfo : accountInfos) {
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
                                            userAsset.contractAddress, accountInfo.address,
                                            context);
                                }
                            }
                        }

                        balancesMultiResponse.setWhenAllCompletedAction(() -> {
                            for (AsyncUtils.GetBalanceResponseBaseContext context : contexts) {
                                String currentAssetSymbol =
                                        context.userAsset.symbol.toLowerCase(Locale.getDefault());
                                Double usdPerThisToken = Utils.getOrDefault(
                                        tokenToUsdRatios, currentAssetSymbol, 0.0d);

                                Double thisBalanceCryptoPart =
                                        context.success ? fromHexWei(context.balance) : 0.0d;

                                Double thisBalanceFiatPart =
                                        usdPerThisToken * thisBalanceCryptoPart;

                                Double prevThisTokenCryptoSum = Utils.getOrDefault(
                                        mPerTokenCryptoSum, currentAssetSymbol, 0.0d);

                                mPerTokenCryptoSum.put(currentAssetSymbol,
                                        prevThisTokenCryptoSum + thisBalanceCryptoPart);

                                Double prevThisTokenFiatSum = Utils.getOrDefault(
                                        mPerTokenFiatSum, currentAssetSymbol, 0.0d);
                                mPerTokenFiatSum.put(currentAssetSymbol,
                                        prevThisTokenFiatSum + thisBalanceFiatPart);

                                mTotalFiatSum += thisBalanceFiatPart;
                            }

                            runWhenDone.run();
                        });
                    }
                });
            });
        });
    }

    private void resetResultData() {
        mUserAssets = new ErcToken[0];
        mTotalFiatSum = 0.0d;
        mPerTokenFiatSum = new HashMap<String, Double>();
        mPerTokenCryptoSum = new HashMap<String, Double>();
    }
}

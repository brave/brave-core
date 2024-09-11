/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Callbacks;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;

public class AssetsPricesHelper {
    private static final String TAG = "AssetsPricesHelper";

    public static void fetchPrices(
            AssetRatioService assetRatioService,
            BlockchainToken[] assets,
            Callbacks.Callback1<HashMap<String, Double>> callback) {
        HashMap<String, Double> assetsPrices = new HashMap<String, Double>();
        AsyncUtils.MultiResponseHandler pricesMultiResponse =
                new AsyncUtils.MultiResponseHandler(assets.length);
        ArrayList<AsyncUtils.GetPriceResponseContext> pricesContexts =
                new ArrayList<AsyncUtils.GetPriceResponseContext>();

        for (BlockchainToken asset : assets) {
            String assetSymbolLower = asset.symbol.toLowerCase(Locale.getDefault());
            String[] fromAssets = new String[] {assetSymbolLower};
            String[] toAssets = new String[] {"usd"};

            AsyncUtils.GetPriceResponseContext priceContext =
                    new AsyncUtils.GetPriceResponseContext(
                            pricesMultiResponse.singleResponseComplete);
            pricesContexts.add(priceContext);

            assetRatioService.getPrice(
                    fromAssets, toAssets, AssetPriceTimeframe.LIVE, priceContext);
        }

        pricesMultiResponse.setWhenAllCompletedAction(
                () -> {
                    for (AsyncUtils.GetPriceResponseContext priceContext : pricesContexts) {
                        if (!priceContext.success || priceContext.prices.length != 1) {
                            continue;
                        }

                        Double usdPerToken = 0.0d;
                        final AssetPrice thisPrice = priceContext.prices[0];
                        final String toConvert = thisPrice.price != null ? thisPrice.price : "0.0";
                        try {
                            usdPerToken = Double.parseDouble(toConvert);
                        } catch (NumberFormatException ex) {
                            Log.e(
                                    TAG,
                                    "Cannot parse "
                                            + toConvert
                                            + ", Token: "
                                            + String.valueOf(thisPrice.fromAsset)
                                            + ", "
                                            + ex);
                            continue;
                        }
                        assetsPrices.put(
                                thisPrice.fromAsset.toLowerCase(Locale.getDefault()), usdPerToken);
                    }
                    callback.call(assetsPrices);
                });
    }
}

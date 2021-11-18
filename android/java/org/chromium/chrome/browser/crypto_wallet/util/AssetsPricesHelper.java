/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Locale;

public class AssetsPricesHelper {
    private static String TAG = "AssetsPricesHelper";
    private AssetRatioController mAssetRatioController;
    private HashSet<String> mAssets;
    private HashMap<String, Double> mAssetsPrices;

    public AssetsPricesHelper(AssetRatioController assetRatioController, HashSet<String> assets) {
        assert assetRatioController != null;
        assert assets != null;
        mAssetRatioController = assetRatioController;
        mAssets = assets;
        mAssetsPrices = new HashMap<String, Double>();
    }

    public HashMap<String, Double> getAssetsPrices() {
        return mAssetsPrices;
    }

    public void fetchPrices(Runnable runWhenDone) {
        AsyncUtils.MultiResponseHandler pricesMultiResponse =
                new AsyncUtils.MultiResponseHandler(mAssets.size());
        ArrayList<AsyncUtils.GetPriceResponseContext> pricesContexts =
                new ArrayList<AsyncUtils.GetPriceResponseContext>();
        Iterator<String> iter = mAssets.iterator();
        while (iter.hasNext()) {
            String asset = iter.next();
            String[] fromAssets = new String[] {asset.toLowerCase(Locale.getDefault())};
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
                String toConvert = "0.0";
                if (priceContext.prices.length == 1) {
                    toConvert = priceContext.prices[0].price;
                }
                try {
                    usdPerToken = Double.parseDouble(toConvert);
                } catch (NullPointerException | NumberFormatException ex) {
                    Log.e(TAG, "Cannot parse " + toConvert + ", " + ex);
                    return;
                }
                mAssetsPrices.put(priceContext.prices[0].fromAsset, usdPerToken);
            }
            runWhenDone.run();
        });
    }
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetPriceRequest;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

public class AssetsPricesHelper {
    private static final String TAG = "AssetsPricesHelper";

    public static void fetchPrices(
            AssetRatioService assetRatioService,
            BlockchainToken[] assets,
            AsyncUtils.Callback1<List<AssetPrice>> callback) {
        List<AssetPriceRequest> requests = new ArrayList<>();

        for (BlockchainToken asset : assets) {
            if (asset == null) {
                continue;
            }

            AssetPriceRequest request = new AssetPriceRequest();
            request.coin = asset.coin;
            request.chainId = asset.chainId;
            request.address = asset.contractAddress.isEmpty() ? null : asset.contractAddress;
            requests.add(request);
        }

        if (requests.isEmpty()) {
            callback.call(new ArrayList<>());
            return;
        }

        AsyncUtils.MultiResponseHandler multiResponse = new AsyncUtils.MultiResponseHandler(1);
        AsyncUtils.GetPriceResponseContext priceContext =
                new AsyncUtils.GetPriceResponseContext(multiResponse.singleResponseComplete);

        multiResponse.setWhenAllCompletedAction(
                () -> {
                    if (priceContext.success && priceContext.prices != null) {
                        callback.call(Arrays.asList(priceContext.prices));
                    } else {
                        Log.e(TAG, "Failed to fetch prices");
                        callback.call(new ArrayList<>());
                    }
                });

        assetRatioService.getPrice(requests.toArray(new AssetPriceRequest[0]), "usd", priceContext);
    }

    /** Core helper method to find the price for a specific asset by coin, chainId, and address. */
    public static double getPrice(
            List<AssetPrice> assetPrices, int coin, String chainId, String address) {
        if (assetPrices == null || assetPrices.isEmpty()) {
            return 0.0d;
        }

        for (AssetPrice assetPrice : assetPrices) {
            // Match by coin, chainId, and address
            if (assetPrice.coin == coin
                    && Objects.equals(assetPrice.chainId, chainId)
                    && Objects.toString(assetPrice.address, "")
                            .equals(Objects.toString(address, ""))) {
                return parseAssetPrice(assetPrice);
            }
        }

        // If no exact match found, return 0
        return 0.0d;
    }

    /** Helper method to find the price for a specific asset from the asset prices list. */
    public static double getPriceForAsset(List<AssetPrice> assetPrices, BlockchainToken asset) {
        if (asset == null) {
            return 0.0d;
        }

        return getPrice(assetPrices, asset.coin, asset.chainId, asset.contractAddress);
    }

    /** Helper method to parse the price string from AssetPrice. */
    private static double parseAssetPrice(AssetPrice assetPrice) {
        if (assetPrice.price != null && !assetPrice.price.isEmpty()) {
            try {
                return Double.parseDouble(assetPrice.price);
            } catch (NumberFormatException ex) {
                Log.e(TAG, "Cannot parse price: " + assetPrice.price, ex);
            }
        }
        return 0.0d;
    }
}

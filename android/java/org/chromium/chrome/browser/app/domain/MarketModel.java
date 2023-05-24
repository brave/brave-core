/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.CoinMarket;

import java.util.Arrays;

public class MarketModel {
    private static final String CURRENCY = "USD";
    // Fetch the first 250 assets.
    private static final byte ASSETS_REQUEST_LIMIT = (byte) (250 & 0xFF);
    private AssetRatioService mAssetRatioService;

    public MarketModel(AssetRatioService assetRatioService) {
        mAssetRatioService = assetRatioService;
    }

    public void resetService(AssetRatioService assetRatioService) {
        mAssetRatioService = assetRatioService;
    }

    public void getCoinMarkets(@NonNull CoinMarketsCallback callback) {
        if (callback == null) {
            throw new IllegalArgumentException("Callback must not be null.");
        }
        if (mAssetRatioService == null) {
            return;
        }
        mAssetRatioService.getCoinMarkets(CURRENCY, ASSETS_REQUEST_LIMIT, (success, coinMarkets) -> {
            if (success) {
                callback.onCoinMarketsSuccess(coinMarkets);
            } else {
                callback.onCoinMarketsFail();
            }

        });
    }

    public interface CoinMarketsCallback {
        void onCoinMarketsSuccess(final CoinMarket[] coinMarkets);
        void onCoinMarketsFail();
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.CoinMarket;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Arrays;
import java.util.Locale;

public class MarketModel {
    private static final String CURRENCY = "USD";
    // Fetch the first 250 assets.
    private static final byte ASSETS_REQUEST_LIMIT = (byte) (250 & 0xFF);
    private AssetRatioService mAssetRatioService;

    private final DecimalFormatSymbols mSymbols;
    private final DecimalFormat mPriceFormatter;
    private final DecimalFormat mPercentageFormatter;

    public MarketModel(AssetRatioService assetRatioService) {
        mAssetRatioService = assetRatioService;

        mSymbols = new DecimalFormatSymbols(Locale.ENGLISH);
        mPriceFormatter = new DecimalFormat("$0.00##########", mSymbols);
        mPriceFormatter.setGroupingSize(3);
        mPriceFormatter.setGroupingUsed(true);

        mPercentageFormatter = new DecimalFormat("0.00", mSymbols);
        mPercentageFormatter.setRoundingMode(RoundingMode.UP);
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
        mAssetRatioService.getCoinMarkets(
                CURRENCY, ASSETS_REQUEST_LIMIT, (success, coinMarkets) -> {
                    if (success) {
                        callback.onCoinMarketsSuccess(coinMarkets);
                    } else {
                        callback.onCoinMarketsFail();
                    }
                });
    }

    @NonNull
    public String getFormattedPrice(double price) {
        return mPriceFormatter.format(price);
    }

    @NonNull
    public String getFormattedPercentageChange(double percentageChange) {
        double absoluteChange = Math.abs(percentageChange);
        return String.format(Locale.ENGLISH, "%s%%", mPercentageFormatter.format(absoluteChange));
    }

    public interface CoinMarketsCallback {
        void onCoinMarketsSuccess(final CoinMarket[] coinMarkets);
        void onCoinMarketsFail();
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.CoinMarket;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

public class MarketModel {
    private static final String CURRENCY = "USD";
    // Fetch the first 250 assets.
    private static final byte ASSETS_REQUEST_LIMIT = (byte) (250 & 0xFF);
    private AssetRatioService mAssetRatioService;

    private final DecimalFormatSymbols mSymbols;
    private final DecimalFormat mPriceFormatter;
    private final DecimalFormat mPercentageFormatter;

    public final LiveData<CoinMarket[]> mCoinMarkets;
    private final MutableLiveData<CoinMarket[]> _mCoinMarkets;

    public MarketModel(AssetRatioService assetRatioService) {
        mAssetRatioService = assetRatioService;

        _mCoinMarkets = new MutableLiveData<>();
        mCoinMarkets = _mCoinMarkets;

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

    /**
     * Gets the first 250 market assets.
     * See {@link #mCoinMarkets} the live data used to emit the assets.
     */
    public void getCoinMarkets() {
        if (mAssetRatioService == null) {
            return;
        }
        mAssetRatioService.getCoinMarkets(
                CURRENCY, ASSETS_REQUEST_LIMIT, (success, coinMarkets) -> {
                    if (success) {
                        _mCoinMarkets.postValue(coinMarkets);
                    } else {
                        _mCoinMarkets.postValue(null);
                    }
                });
    }

    /**
     * Gets formatted price using the format `$0.00##########` (e.g. $27,160.00, or
     * $0.523453456789).
     * @param price Price to format.
     * @return Formatted price.
     */
    @NonNull
    public String getFormattedPrice(double price) {
        return mPriceFormatter.format(price);
    }

    /**
     * Gets formatted percentage change rounded up and using the format `0.00` (e.g. 0.05%).
     * @param percentageChange Percentage change to format.
     * @return Formatted percentage change.
     */
    @NonNull
    public String getFormattedPercentageChange(double percentageChange) {
        double absoluteChange = Math.abs(percentageChange);
        return String.format(Locale.ENGLISH, "%s%%", mPercentageFormatter.format(absoluteChange));
    }

    /**
     * Formats a given USD value into a more readable string appending K (thousands),
     * M (millions), or B (billions).
     * E.g.{@code 1000000} becomes {@code "$1.00M"}.
     *
     * @param value Given value to format.
     * @return Formatted string.
     */
    @NonNull
    public String getFormattedUsdBillions(final double value) {
        if (value < 1000) {
            final int rounded = (int) value;
            return String.format(Locale.ENGLISH, "$%d", rounded);
        }
        final int exp = Math.min(3, (int) (Math.log(value) / Math.log(1000)));
        return String.format(
                Locale.ENGLISH, "$%.2f%c", value / Math.pow(1000, exp), "KMB".charAt(exp - 1));
    }
}

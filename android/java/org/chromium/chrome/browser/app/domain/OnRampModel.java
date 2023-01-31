/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.res.Resources;
import android.text.TextUtils;

import androidx.core.os.ConfigurationCompat;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.OnRampProvider;

import java.util.Locale;

public class OnRampModel {
    private static final String CURRENCY_CODE_USD = "USD";
    private final Object mLock = new Object();
    private AssetRatioService mAssetRatioService;

    public OnRampModel(AssetRatioService assetRatioService) {
        mAssetRatioService = assetRatioService;
    }

    public void getBuyUrl(int onRampProvider, String chainId, String from, String rampNetworkSymbol,
            String amount, Resources resources, OnRampCallback callback) {
        if (onRampProvider == OnRampProvider.SARDINE) {
            // Sardine services are available only for US locales.
            Locale currentLocale =
                    ConfigurationCompat.getLocales(resources.getConfiguration()).get(0);
            if (currentLocale == null || !currentLocale.getCountry().equals("US")) {
                callback.OnUrlReady(null);
                return;
            }
        }
        mAssetRatioService.getBuyUrlV1(onRampProvider, chainId, from, rampNetworkSymbol, amount,
                CURRENCY_CODE_USD, (url, error) -> {
                    if (error != null && !error.isEmpty()) {
                        callback.OnUrlReady(null);
                        return;
                    }
                    callback.OnUrlReady(url);
                });
    }

    void resetServices(AssetRatioService assetRatioService) {
        synchronized (mLock) {
            mAssetRatioService = assetRatioService;
        }
    }

    public interface OnRampCallback {
        void OnUrlReady(String url);
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.res.Resources;

import androidx.core.os.ConfigurationCompat;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OnRampProvider;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.mojo.bindings.Callbacks.Callback1;

import java.util.Locale;

public class BuyModel {
    public static final int[] SUPPORTED_RAMP_PROVIDERS = {
            OnRampProvider.RAMP, OnRampProvider.SARDINE, OnRampProvider.TRANSAK};

    private final Object mLock = new Object();
    private AssetRatioService mAssetRatioService;
    private BlockchainRegistry mBlockchainRegistry;

    public BuyModel(AssetRatioService assetRatioService, BlockchainRegistry blockchainRegistry) {
        mAssetRatioService = assetRatioService;
        mBlockchainRegistry = blockchainRegistry;
    }

    public boolean isAvailable(int onRampProvider, Resources resources) {
        if (onRampProvider == OnRampProvider.SARDINE) {
            // Sardine services are available only for US locales.
            Locale currentLocale =
                    ConfigurationCompat.getLocales(resources.getConfiguration()).get(0);
            return (currentLocale != null && currentLocale.getCountry().equals("US"));
        }
        return true;
    }

    public void getBuyUrl(int onRampProvider, String chainId, String from, String symbol,
            String amount, String contractAddress, OnRampCallback callback) {
        if (onRampProvider == OnRampProvider.RAMP) {
            // Ramp.Network is a special case that requires a modified asset symbol.
            symbol = AssetUtils.mapToRampNetworkSymbol(chainId, symbol, contractAddress);
        }
        mAssetRatioService.getBuyUrlV1(onRampProvider, chainId, from, symbol, amount,
                WalletConstants.CURRENCY_CODE_USD, (url, error) -> {
                    if (error != null && !error.isEmpty()) {
                        callback.OnUrlReady(null);
                        return;
                    }
                    callback.OnUrlReady(url);
                });
    }

    public void isBuySupported(NetworkInfo selectedNetwork, String assetSymbol,
            String contractAddress, String chainId, int[] rampProviders,
            Callback1<Boolean> callback) {
        TokenUtils.getBuyTokensFiltered(mBlockchainRegistry, selectedNetwork,
                TokenUtils.TokenType.ALL, rampProviders, tokens -> {
                    callback.call(JavaUtils.includes(tokens,
                            iToken
                            -> AssetUtils.Filters.isSameToken(
                                    iToken, assetSymbol, contractAddress, chainId)));
                });
    }

    void resetServices(AssetRatioService assetRatioService, BlockchainRegistry blockchainRegistry) {
        synchronized (mLock) {
            mAssetRatioService = assetRatioService;
            mBlockchainRegistry = blockchainRegistry;
        }
    }

    public interface OnRampCallback {
        void OnUrlReady(String url);
    }
}

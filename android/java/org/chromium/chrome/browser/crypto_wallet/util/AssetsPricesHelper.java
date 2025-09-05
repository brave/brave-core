/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;

import java.util.HashMap;

public class AssetsPricesHelper {
    private static final String TAG = "AssetsPricesHelper";

    public static void fetchPrices(
            AssetRatioService assetRatioService,
            BlockchainToken[] assets,
            Callbacks.Callback1<HashMap<String, Double>> callback) {
        // Method is not used - return empty HashMap to maintain compatibility
        HashMap<String, Double> assetsPrices = new HashMap<String, Double>();
        callback.call(assetsPrices);
    }
}

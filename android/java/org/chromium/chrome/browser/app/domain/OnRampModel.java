/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.OnRampProvider;

public class OnRampModel {
    private final BlockchainRegistry mBlockchainRegistry;

    public OnRampModel(BlockchainRegistry blockchainRegistry) {
        mBlockchainRegistry = blockchainRegistry;
    }

    public void getBuyUrl(int onRampProvider, String chainId, String from, String rampNetworkSymbol, String amount, OnRampCallback callback) {
         mBlockchainRegistry.getBuyUrl(OnRampProvider.RAMP, chainId,
                        from, rampNetworkSymbol, amount, (url, error) -> {
                            if (error != null && !error.isEmpty()) {
                                callback.OnUrlReady(null);
                                return;
                            }
                            callback.OnUrlReady(url);
                        });
    }

    public interface OnRampCallback {
        void OnUrlReady(String url);
    }
}

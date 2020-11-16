/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.widget.crypto.binance;

public class ConvertAsset {
    private String mAsset;
    private String mMinAmount;

    public ConvertAsset(String asset, String minAmount) {
        this.mAsset = asset;
        this.mMinAmount = minAmount;
    }

    public String getAsset() {
        return mAsset;
    }

    public String getMinAmount() {
        return mMinAmount;
    }
}
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.billing;

import com.android.billingclient.api.Purchase;

public class PurchaseModel {
    private String mPurchaseToken;
    private String mProductId;
    private Purchase mPurchase;

    public PurchaseModel(String purchaseToken, String productId, Purchase purchase) {
        this.mPurchaseToken = purchaseToken;
        this.mProductId = productId;
        this.mPurchase = purchase;
    }

    public String getPurchaseToken() {
        return mPurchaseToken;
    }

    public String getProductId() {
        return mProductId;
    }

    public Purchase getPurchase() {
        return mPurchase;
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.PurchaseModel;
import org.chromium.chrome.browser.util.LiveDataUtil;

public class BraveLeoUtils {
    public static void verifySubscription(Callback callback) {
        MutableLiveData<PurchaseModel> _activePurchases = new MutableLiveData();
        LiveData<PurchaseModel> activePurchases = _activePurchases;
        InAppPurchaseWrapper.getInstance()
                .queryPurchases(_activePurchases, InAppPurchaseWrapper.SubscriptionProduct.LEO);
        LiveDataUtil.observeOnce(
                activePurchases,
                activePurchaseModel -> {
                    BraveLeoPrefUtils.setIsSubscriptionActive(activePurchaseModel != null);
                    if (activePurchaseModel != null) {
                    } else {
                    }
                    if (callback != null) {
                        callback.onResult(null);
                    }
                });
    }
}

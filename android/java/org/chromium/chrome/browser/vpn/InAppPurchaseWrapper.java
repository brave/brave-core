/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import static com.android.billingclient.api.BillingClient.BillingResponseCode.OK;
import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.NonNull;

import com.android.billingclient.api.AcknowledgePurchaseParams;
import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;

import org.chromium.base.Log;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class InAppPurchaseWrapper {
    public static final String NIGHTLY_MONTHLY_SUBSCRIPTION = "nightly.bravevpn.monthly";
    public static final String NIGHTLY_YEARLY_SUBSCRIPTION = "nightly.bravevpn.yearly";
    private static InAppPurchaseWrapper inAppPurchaseWrapper = null;
    private BillingClient billingClient;
    private Context context;

    private final Map<String, SkuDetails> skusWithSkuDetails = new HashMap<>();

    public SkuDetails getSkuDetails(String sku) {
        return skusWithSkuDetails.get(sku);
    }

    public static final List<String> SUBS_SKUS = new ArrayList<>(
            Arrays.asList(NIGHTLY_MONTHLY_SUBSCRIPTION, NIGHTLY_YEARLY_SUBSCRIPTION));

    public static InAppPurchaseWrapper getInstance() {
        if (inAppPurchaseWrapper == null) inAppPurchaseWrapper = new InAppPurchaseWrapper();

        return inAppPurchaseWrapper;
    }

    public void startBillingServiceConnection(Context context) {
        this.context = context;
        billingClient = BillingClient.newBuilder(context)
                                .enablePendingPurchases()
                                .setListener(purchasesUpdatedListener)
                                .build();

        connectToBillingService();
    }

    public void connectToBillingService() {
        if (!billingClient.isReady()) {
            billingClient.startConnection(billingClientStateListener);
        }
    }

    public BillingClient getBillingClient() {
        return billingClient;
    }

    public void querySkuDetailsAsync(List<String> skuList) {
        SkuDetailsParams params = SkuDetailsParams.newBuilder()
                                          .setSkusList(skuList)
                                          .setType(BillingClient.SkuType.SUBS)
                                          .build();
        billingClient.querySkuDetailsAsync(params, (billingResult, skuDetailsList) -> {
            if (billingResult.getResponseCode() == OK && skuDetailsList != null) {
                for (SkuDetails skuDetails : skuDetailsList) {
                    skusWithSkuDetails.put(skuDetails.getSku(), skuDetails);
                }
            }
        });
    }

    public List<Purchase> queryPurchases() {
        return billingClient.queryPurchases(SUBS).getPurchasesList();
    }

    public void purchase(Activity activity, SkuDetails skuDetails) {
        BillingFlowParams billingFlowParams =
                BillingFlowParams.newBuilder().setSkuDetails(skuDetails).build();

        int responseCode =
                billingClient.launchBillingFlow(activity, billingFlowParams).getResponseCode();
    }

    public void processPurchases(List<Purchase> purchases) {
        for (Purchase purchase : purchases) {
            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                acknowledgePurchase(purchase);
            }
        }
    }

    private void acknowledgePurchase(Purchase purchase) {
        AcknowledgePurchaseParams acknowledgePurchaseParams =
                AcknowledgePurchaseParams.newBuilder()
                        .setPurchaseToken(purchase.getPurchaseToken())
                        .build();
        if (!purchase.isAcknowledged()) {
            billingClient.acknowledgePurchase(acknowledgePurchaseParams, billingResult -> {
                if (billingResult.getResponseCode() == OK) {
                    BraveVpnPrefUtils.setBraveVpnBooleanPref(
                            BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, true);
                    Log.e("BraveVPN", "Subscription consumed");
                    BraveVpnUtils.openBraveVpnProfileActivity(context);
                    Toast.makeText(context, "Subscription successfully consumed", Toast.LENGTH_LONG)
                            .show();
                } else {
                    Toast.makeText(context, "Failed to acknowledge purchase :" + billingResult,
                                 Toast.LENGTH_LONG)
                            .show();
                }
            });
        } else {
            BraveVpnPrefUtils.setBraveVpnBooleanPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, true);
        }
    }

    PurchasesUpdatedListener purchasesUpdatedListener = (billingResult, purchases) -> {
        if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
            if (purchases != null) processPurchases(purchases);
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            Toast.makeText(context,
                         "You are already subscribed to the subscription. Try restoring the subscription.",
                         Toast.LENGTH_LONG)
                    .show();
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.SERVICE_DISCONNECTED) {
            InAppPurchaseWrapper.getInstance().connectToBillingService();
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.USER_CANCELED) {
            Toast.makeText(context, "ERROR!!\nCaused by a user cancelling the purchase flow.",
                         Toast.LENGTH_LONG)
                    .show();
        } else {
            Toast.makeText(context, "ERROR!!\nPurchased failed..", Toast.LENGTH_SHORT).show();
        }
    };

    BillingClientStateListener billingClientStateListener = new BillingClientStateListener() {
        @Override
        public void onBillingServiceDisconnected() {
            InAppPurchaseWrapper.getInstance().connectToBillingService();
        }

        @Override
        public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                InAppPurchaseWrapper.getInstance().querySkuDetailsAsync(SUBS_SKUS);
            }
        }
    };
}
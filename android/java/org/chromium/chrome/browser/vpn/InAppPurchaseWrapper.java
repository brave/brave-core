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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class InAppPurchaseWrapper {
    public static final String NIGHTLY_MONTHLY_SUBSCRIPTION = "nightly.bravevpn.monthly";
    public static final String NIGHTLY_YEARLY_SUBSCRIPTION = "nightly.bravevpn.yearly";
    private static InAppPurchaseWrapper sInAppPurchaseWrapper;
    private BillingClient mBillingClient;
    private Context mContext;

    private final Map<String, SkuDetails> mSkusWithSkuDetails = new HashMap<>();

    public SkuDetails getSkuDetails(String sku) {
        return mSkusWithSkuDetails.get(sku);
    }

    public static final List<String> SUBS_SKUS = new ArrayList<>(
            Arrays.asList(NIGHTLY_MONTHLY_SUBSCRIPTION, NIGHTLY_YEARLY_SUBSCRIPTION));

    public static InAppPurchaseWrapper getInstance() {
        if (sInAppPurchaseWrapper == null) sInAppPurchaseWrapper = new InAppPurchaseWrapper();

        return sInAppPurchaseWrapper;
    }

    public void startBillingServiceConnection(Context context) {
        this.mContext = context;
        mBillingClient = BillingClient.newBuilder(context)
                                 .enablePendingPurchases()
                                 .setListener(purchasesUpdatedListener)
                                 .build();

        connectToBillingService();
    }

    public void connectToBillingService() {
        if (!mBillingClient.isReady()) {
            mBillingClient.startConnection(billingClientStateListener);
        }
    }

    public BillingClient getBillingClient() {
        return mBillingClient;
    }

    public void querySkuDetailsAsync(List<String> skuList) {
        SkuDetailsParams params = SkuDetailsParams.newBuilder()
                                          .setSkusList(skuList)
                                          .setType(BillingClient.SkuType.SUBS)
                                          .build();
        mBillingClient.querySkuDetailsAsync(params, (billingResult, skuDetailsList) -> {
            if (billingResult.getResponseCode() == OK && skuDetailsList != null) {
                for (SkuDetails skuDetails : skuDetailsList) {
                    mSkusWithSkuDetails.put(skuDetails.getSku(), skuDetails);
                }
            }
        });
    }

    public List<Purchase> queryPurchases() {
        return mBillingClient.queryPurchases(SUBS).getPurchasesList();
    }

    public void purchase(Activity activity, SkuDetails skuDetails) {
        BillingFlowParams billingFlowParams =
                BillingFlowParams.newBuilder().setSkuDetails(skuDetails).build();

        int responseCode =
                mBillingClient.launchBillingFlow(activity, billingFlowParams).getResponseCode();
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
            mBillingClient.acknowledgePurchase(acknowledgePurchaseParams, billingResult -> {
                if (billingResult.getResponseCode() == OK) {
                    BraveVpnPrefUtils.setSubscriptionPurchase(true);
                    BraveVpnUtils.openBraveVpnProfileActivity(mContext);
                    Toast.makeText(mContext,
                                 mContext.getResources().getString(R.string.subscription_consumed),
                                 Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Toast.makeText(mContext,
                                 mContext.getResources().getString(R.string.fail_to_aknowledge)
                                         + billingResult,
                                 Toast.LENGTH_SHORT)
                            .show();
                }
            });
        } else {
            BraveVpnPrefUtils.setSubscriptionPurchase(true);
        }
    }

    PurchasesUpdatedListener purchasesUpdatedListener = (billingResult, purchases) -> {
        if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
            if (purchases != null) processPurchases(purchases);
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            Toast.makeText(mContext, mContext.getResources().getString(R.string.already_subscribed),
                         Toast.LENGTH_SHORT)
                    .show();
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.SERVICE_DISCONNECTED) {
            connectToBillingService();
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.USER_CANCELED) {
            Toast.makeText(mContext,
                         mContext.getResources().getString(R.string.error_caused_by_user),
                         Toast.LENGTH_SHORT)
                    .show();
        } else {
            Toast.makeText(mContext, mContext.getResources().getString(R.string.purchased_failed),
                         Toast.LENGTH_SHORT)
                    .show();
        }
    };

    BillingClientStateListener billingClientStateListener = new BillingClientStateListener() {
        @Override
        public void onBillingServiceDisconnected() {
            connectToBillingService();
        }

        @Override
        public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                querySkuDetailsAsync(SUBS_SKUS);
            }
        }
    };
}
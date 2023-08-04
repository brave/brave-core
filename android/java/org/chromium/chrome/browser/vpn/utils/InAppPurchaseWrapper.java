/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

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
import com.android.billingclient.api.PurchasesResponseListener;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class InAppPurchaseWrapper {
    private static final String TAG = "InAppPurchaseWrapper";
    public static final String NIGHTLY_MONTHLY_SUBSCRIPTION = "nightly.bravevpn.monthly";
    public static final String NIGHTLY_YEARLY_SUBSCRIPTION = "nightly.bravevpn.yearly";

    public static final String RELEASE_MONTHLY_SUBSCRIPTION = "brave.vpn.monthly";
    public static final String RELEASE_YEARLY_SUBSCRIPTION = "brave.vpn.yearly";
    private BillingClient mBillingClient;
    private int mRetryCount;

    private final Map<String, SkuDetails> mSkusWithSkuDetails = new HashMap<>();

    public SkuDetails getSkuDetails(String sku) {
        return mSkusWithSkuDetails.get(sku);
    }

    public static final List<String> NIGHTLY_SUBS_SKUS = new ArrayList<>(
            Arrays.asList(NIGHTLY_MONTHLY_SUBSCRIPTION, NIGHTLY_YEARLY_SUBSCRIPTION));

    public static final List<String> RELEASE_SUBS_SKUS = new ArrayList<>(
            Arrays.asList(RELEASE_MONTHLY_SUBSCRIPTION, RELEASE_YEARLY_SUBSCRIPTION));

    private static volatile InAppPurchaseWrapper sInAppPurchaseWrapper;
    private static Object mutex = new Object();

    private InAppPurchaseWrapper() {}

    public static InAppPurchaseWrapper getInstance() {
        InAppPurchaseWrapper result = sInAppPurchaseWrapper;
        if (result == null) {
            synchronized (mutex) {
                result = sInAppPurchaseWrapper;
                if (result == null) sInAppPurchaseWrapper = result = new InAppPurchaseWrapper();
            }
        }
        return result;
    }

    public boolean isMonthlySubscription(String productId) {
        return productId.equals(NIGHTLY_MONTHLY_SUBSCRIPTION)
                || productId.equals(RELEASE_MONTHLY_SUBSCRIPTION);
    }

    public void startBillingServiceConnection(Context context) {
        mBillingClient = BillingClient.newBuilder(context)
                                 .enablePendingPurchases()
                                 .setListener(getPurchasesUpdatedListener(context))
                                 .build();

        connectToBillingService();
    }

    public void connectToBillingService() {
        if (!mBillingClient.isReady()) {
            try {
                mBillingClient.startConnection(billingClientStateListener);
            } catch (IllegalStateException exc) {
                // That prevents a crash that some users experience
                // https://github.com/brave/brave-browser/issues/27751.
                // It's unknown what causes it, we tried to add retries, but it
                // didn't help.
                Log.e(TAG, "connectToBillingService " + exc.getMessage());
            }
        }
    }

    public boolean isSubscriptionSupported() {
        if (mBillingClient != null) {
            BillingResult result =
                    mBillingClient.isFeatureSupported(BillingClient.FeatureType.SUBSCRIPTIONS);
            BraveRewardsNativeWorker braveRewardsNativeWorker =
                    BraveRewardsNativeWorker.getInstance();
            return (result.getResponseCode() == BillingClient.BillingResponseCode.OK
                    && braveRewardsNativeWorker != null && braveRewardsNativeWorker.IsSupported());
        }
        return false;
    }

    private void querySkuDetailsAsync(List<String> skuList) {
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

    public void queryPurchases(PurchasesResponseListener purchasesResponseListener) {
        mBillingClient.queryPurchasesAsync(SUBS, purchasesResponseListener);
    }

    public void purchase(Activity activity, SkuDetails skuDetails) {
        BillingFlowParams billingFlowParams =
                BillingFlowParams.newBuilder().setSkuDetails(skuDetails).build();

        int responseCode =
                mBillingClient.launchBillingFlow(activity, billingFlowParams).getResponseCode();
    }

    public void processPurchases(Context context, List<Purchase> purchases) {
        for (Purchase purchase : purchases) {
            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                acknowledgePurchase(context, purchase);
            }
        }
    }

    private void acknowledgePurchase(Context context, Purchase purchase) {
        AcknowledgePurchaseParams acknowledgePurchaseParams =
                AcknowledgePurchaseParams.newBuilder()
                        .setPurchaseToken(purchase.getPurchaseToken())
                        .build();
        if (!purchase.isAcknowledged()) {
            mBillingClient.acknowledgePurchase(acknowledgePurchaseParams, billingResult -> {
                if (billingResult.getResponseCode() == OK) {
                    BraveVpnPrefUtils.setSubscriptionPurchase(true);
                    BraveVpnUtils.openBraveVpnProfileActivity(context);
                    Toast.makeText(context,
                                 context.getResources().getString(R.string.subscription_consumed),
                                 Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Toast.makeText(context,
                                 context.getResources().getString(R.string.fail_to_aknowledge)
                                         + billingResult,
                                 Toast.LENGTH_SHORT)
                            .show();
                }
            });
        } else {
            BraveVpnPrefUtils.setSubscriptionPurchase(true);
        }
    }

    private PurchasesUpdatedListener getPurchasesUpdatedListener(Context context) {
        return (billingResult, purchases) -> {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                if (purchases != null) {
                    mRetryCount = 0;
                    processPurchases(context, purchases);
                }
            } else if (billingResult.getResponseCode()
                    == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
                Toast.makeText(context,
                             context.getResources().getString(R.string.already_subscribed),
                             Toast.LENGTH_SHORT)
                        .show();
            } else if (billingResult.getResponseCode()
                            == BillingClient.BillingResponseCode.SERVICE_DISCONNECTED
                    && mRetryCount < 5) {
                connectToBillingService();
                mRetryCount++;
            } else if (billingResult.getResponseCode()
                    == BillingClient.BillingResponseCode.USER_CANCELED) {
                Toast.makeText(context,
                             context.getResources().getString(R.string.error_caused_by_user),
                             Toast.LENGTH_SHORT)
                        .show();
            } else {
                Toast.makeText(context, context.getResources().getString(R.string.purchased_failed),
                             Toast.LENGTH_SHORT)
                        .show();
            }
        };
    }

    BillingClientStateListener billingClientStateListener = new BillingClientStateListener() {
        private int retryCount;
        @Override
        public void onBillingServiceDisconnected() {
            retryCount++;
            if (retryCount <= 3) {
                connectToBillingService();
            }
        }
        @Override
        public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                retryCount = 0;
                querySkuDetailsAsync(NIGHTLY_SUBS_SKUS);
                querySkuDetailsAsync(RELEASE_SUBS_SKUS);
            }
        }
    };
}
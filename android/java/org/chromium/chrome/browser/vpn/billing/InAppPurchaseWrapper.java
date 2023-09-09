/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.billing;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.android.billingclient.api.AcknowledgePurchaseParams;
import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryPurchasesParams;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

import java.util.ArrayList;
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

    private static volatile InAppPurchaseWrapper sInAppPurchaseWrapper;
    private static Object mutex = new Object();

    public enum SubscriptionType { MONTHLY, YEARLY }

    private MutableLiveData<Purchase> mutableActivePurchase = new MutableLiveData();
    private LiveData<Purchase> activePurchase = mutableActivePurchase;
    private void setActivePurchase(Purchase purchase) {
        mutableActivePurchase.postValue(purchase);
    }

    public LiveData<Purchase> getActivePurchase() {
        return activePurchase;
    }

    private MutableLiveData<ProductDetails> mutableMonthlyProductDetails = new MutableLiveData();
    private LiveData<ProductDetails> monthlyProductDetails = mutableMonthlyProductDetails;
    private void setMonthlyProductDetails(ProductDetails productDetails) {
        mutableMonthlyProductDetails.postValue(productDetails);
    }
    public LiveData<ProductDetails> getMonthlyProductDetails() {
        return monthlyProductDetails;
    }

    private MutableLiveData<ProductDetails> mutableYearlyProductDetails = new MutableLiveData();
    private LiveData<ProductDetails> yearlyProductDetails = mutableYearlyProductDetails;
    private void setYearlyProductDetails(ProductDetails productDetails) {
        mutableYearlyProductDetails.postValue(productDetails);
    }
    public LiveData<ProductDetails> getYearlyProductDetails() {
        return yearlyProductDetails;
    }

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

    public boolean isBillingClientReady() {
        return mBillingClient != null && mBillingClient.isReady();
    }

    public void startBillingServiceConnection(
            Context context, MutableLiveData<Boolean> billingClientConnectionState) {
        if (!BraveVpnUtils.isBraveVpnFeatureEnable()) {
            return;
        }
        mBillingClient = BillingClient.newBuilder(context)
                                 .enablePendingPurchases()
                                 .setListener(getPurchasesUpdatedListener(context))
                                 .build();
        if (!mBillingClient.isReady()) {
            try {
                mBillingClient.startConnection(new BillingClientStateListener() {
                    private int retryCount;
                    @Override
                    public void onBillingServiceDisconnected() {
                        retryCount++;
                        if (retryCount <= 3) {
                            startBillingServiceConnection(context, billingClientConnectionState);
                        }
                    }
                    @Override
                    public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                        if (billingResult.getResponseCode()
                                == BillingClient.BillingResponseCode.OK) {
                            retryCount = 0;
                            if (billingClientConnectionState != null) {
                                billingClientConnectionState.postValue(true);
                            }
                        }
                    }
                });
            } catch (IllegalStateException exc) {
                // That prevents a crash that some users experience
                // https://github.com/brave/brave-browser/issues/27751.
                // It's unknown what causes it, we tried to add retries, but it
                // didn't help.
                Log.e(TAG, "startBillingServiceConnection " + exc.getMessage());
            }
        } else {
            if (billingClientConnectionState != null) {
                billingClientConnectionState.postValue(mBillingClient.getConnectionState()
                        == BillingClient.ConnectionState.CONNECTED);
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

    public String getProductId(SubscriptionType subscriptionType) {
        boolean isReleaseBuild = ContextUtils.getApplicationContext().getPackageName().equals(
                BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME);
        if (isReleaseBuild) {
            return subscriptionType == SubscriptionType.MONTHLY ? RELEASE_MONTHLY_SUBSCRIPTION
                                                                : RELEASE_YEARLY_SUBSCRIPTION;
        } else {
            return subscriptionType == SubscriptionType.MONTHLY ? NIGHTLY_MONTHLY_SUBSCRIPTION
                                                                : NIGHTLY_YEARLY_SUBSCRIPTION;
        }
    }

    public void queryProductDetailsAsync() {
        Map<String, ProductDetails> productDetails = new HashMap<>();
        List<QueryProductDetailsParams.Product> products = new ArrayList<>();
        products.add(QueryProductDetailsParams.Product.newBuilder()
                             .setProductId(getProductId(SubscriptionType.MONTHLY))
                             .setProductType(BillingClient.ProductType.SUBS)
                             .build());
        products.add(QueryProductDetailsParams.Product.newBuilder()
                             .setProductId(getProductId(SubscriptionType.YEARLY))
                             .setProductType(BillingClient.ProductType.SUBS)
                             .build());
        QueryProductDetailsParams queryProductDetailsParams =
                QueryProductDetailsParams.newBuilder().setProductList(products).build();

        mBillingClient.queryProductDetailsAsync(
                queryProductDetailsParams, (billingResult, productDetailsList) -> {
                    if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                        for (ProductDetails productDetail : productDetailsList) {
                            productDetails.put(productDetail.getProductId(), productDetail);
                        }
                        setMonthlyProductDetails(
                                productDetails.get(getProductId(SubscriptionType.MONTHLY)));
                        setYearlyProductDetails(
                                productDetails.get(getProductId(SubscriptionType.YEARLY)));
                    } else {
                        Log.e(TAG,
                                "queryProductDetailsAsync failed"
                                        + billingResult.getDebugMessage());
                    }
                });
    }

    public void queryPurchases() {
        mBillingClient.queryPurchasesAsync(QueryPurchasesParams.newBuilder()
                                                   .setProductType(BillingClient.ProductType.SUBS)
                                                   .build(),
                (billingResult, purchases) -> {
                    Purchase activePurchase = null;
                    if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                        for (Purchase purchase : purchases) {
                            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                                activePurchase = purchase;
                                break;
                            }
                        }
                    } else {
                        Log.e(TAG, "queryPurchases failed" + billingResult.getDebugMessage());
                    }
                    setActivePurchase(activePurchase);
                });
    }

    public void initiatePurchase(Activity activity, ProductDetails productDetails) {
        String offerToken = productDetails.getSubscriptionOfferDetails().get(0).getOfferToken();
        List<BillingFlowParams.ProductDetailsParams> productDetailsParamsList = new ArrayList<>();
        productDetailsParamsList.add(BillingFlowParams.ProductDetailsParams.newBuilder()
                                             .setProductDetails(productDetails)
                                             .setOfferToken(offerToken)
                                             .build());

        BillingFlowParams billingFlowParams =
                BillingFlowParams.newBuilder()
                        .setProductDetailsParamsList(productDetailsParamsList)
                        .build();

        BillingResult billingResult = mBillingClient.launchBillingFlow(activity, billingFlowParams);
    }

    public void processPurchases(Context context, Purchase activePurchase) {
        acknowledgePurchase(context, activePurchase);
    }

    private void acknowledgePurchase(Context context, Purchase purchase) {
        AcknowledgePurchaseParams acknowledgePurchaseParams =
                AcknowledgePurchaseParams.newBuilder()
                        .setPurchaseToken(purchase.getPurchaseToken())
                        .build();
        if (!purchase.isAcknowledged()) {
            mBillingClient.acknowledgePurchase(acknowledgePurchaseParams, billingResult -> {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    BraveVpnPrefUtils.setSubscriptionPurchase(true);
                    BraveVpnUtils.openBraveVpnProfileActivity(context);
                    BraveVpnUtils.showToast(
                            context.getResources().getString(R.string.subscription_consumed));
                } else {
                    BraveVpnUtils.showToast(
                            context.getResources().getString(R.string.fail_to_aknowledge));
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
                    for (Purchase purchase : purchases) {
                        if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                            processPurchases(context, purchase);
                        }
                    }
                }
            } else if (billingResult.getResponseCode()
                    == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
                BraveVpnUtils.showToast(
                        context.getResources().getString(R.string.already_subscribed));
            } else if (billingResult.getResponseCode()
                            == BillingClient.BillingResponseCode.SERVICE_DISCONNECTED
                    && mRetryCount < 5) {
                startBillingServiceConnection(context, null);
                mRetryCount++;
            } else if (billingResult.getResponseCode()
                    == BillingClient.BillingResponseCode.USER_CANCELED) {
                BraveVpnUtils.showToast(
                        context.getResources().getString(R.string.error_caused_by_user));
            } else {
                BraveVpnUtils.showToast(
                        context.getResources().getString(R.string.purchased_failed));
            }
        };
    }
}

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
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class InAppPurchaseWrapper {
    private static final String TAG = "InAppPurchaseWrapper";
    public static final String NIGHTLY_MONTHLY_SUBSCRIPTION = "nightly.bravevpn.monthly";
    public static final String NIGHTLY_YEARLY_SUBSCRIPTION = "nightly.bravevpn.yearly";

    public static final String RELEASE_MONTHLY_SUBSCRIPTION = "brave.vpn.monthly";
    public static final String RELEASE_YEARLY_SUBSCRIPTION = "brave.vpn.yearly";
    private BillingClient mBillingClient;

    private static final long MICRO_UNITS =
            1000000; // 1,000,000 micro-units equal one unit of the currency

    private static volatile InAppPurchaseWrapper sInAppPurchaseWrapper;
    private static Object sMutex = new Object();

    public enum SubscriptionType { MONTHLY, YEARLY }

    private MutableLiveData<ProductDetails> mMutableMonthlyProductDetails = new MutableLiveData();
    private LiveData<ProductDetails> mMonthlyProductDetails = mMutableMonthlyProductDetails;
    private void setMonthlyProductDetails(ProductDetails productDetails) {
        mMutableMonthlyProductDetails.postValue(productDetails);
    }
    public LiveData<ProductDetails> getMonthlyProductDetails() {
        return mMonthlyProductDetails;
    }

    private MutableLiveData<ProductDetails> mMutableYearlyProductDetails = new MutableLiveData();
    private LiveData<ProductDetails> mYearlyProductDetails = mMutableYearlyProductDetails;
    private void setYearlyProductDetails(ProductDetails productDetails) {
        mMutableYearlyProductDetails.postValue(productDetails);
    }
    public LiveData<ProductDetails> getYearlyProductDetails() {
        return mYearlyProductDetails;
    }

    private InAppPurchaseWrapper() {}

    public static InAppPurchaseWrapper getInstance() {
        InAppPurchaseWrapper result = sInAppPurchaseWrapper;
        if (result == null) {
            synchronized (sMutex) {
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

    private void startBillingServiceConnection(
            MutableLiveData<Boolean> billingClientConnectionState) {
        Context context = ContextUtils.getApplicationContext();
        if (!BraveVpnUtils.isVpnFeatureSupported(context)) {
            return;
        }

        // End existing connection if any before we start another connection
        endConnection();

        mBillingClient = BillingClient.newBuilder(context)
                                 .enablePendingPurchases()
                                 .setListener(getPurchasesUpdatedListener(context))
                                 .build();
        if (!mBillingClient.isReady()) {
            try {
                mBillingClient.startConnection(new BillingClientStateListener() {
                    @Override
                    public void onBillingServiceDisconnected() {
                        retryBillingServiceConnection(billingClientConnectionState);
                    }
                    @Override
                    public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                        if (billingResult.getResponseCode()
                                == BillingClient.BillingResponseCode.OK) {
                            if (billingClientConnectionState != null) {
                                billingClientConnectionState.postValue(true);
                            }
                        } else {
                            BraveVpnUtils.showToast(billingResult.getDebugMessage());
                            retryBillingServiceConnection(billingClientConnectionState);
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

    private void endConnection() {
        if (mBillingClient != null) {
            mBillingClient.endConnection();
            mBillingClient = null;
        }
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

        MutableLiveData<Boolean> _billingConnectionState = new MutableLiveData();
        LiveData<Boolean> billingConnectionState = _billingConnectionState;
        startBillingServiceConnection(_billingConnectionState);
        LiveDataUtil.observeOnce(billingConnectionState, isConnected -> {
            if (isConnected) {
                mBillingClient.queryProductDetailsAsync(
                        queryProductDetailsParams, (billingResult, productDetailsList) -> {
                            // End connection after getting the product details
                            endConnection();

                            if (billingResult.getResponseCode()
                                    == BillingClient.BillingResponseCode.OK) {
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
                                BraveVpnUtils.showToast(billingResult.getDebugMessage());
                            }
                        });
            }
        });
    }

    public void queryPurchases(MutableLiveData<PurchaseModel> mutableActivePurchases) {
        MutableLiveData<Boolean> _billingConnectionState = new MutableLiveData();
        LiveData<Boolean> billingConnectionState = _billingConnectionState;
        startBillingServiceConnection(_billingConnectionState);
        LiveDataUtil.observeOnce(billingConnectionState, isConnected -> {
            if (isConnected) {
                mBillingClient.queryPurchasesAsync(
                        QueryPurchasesParams.newBuilder()
                                .setProductType(BillingClient.ProductType.SUBS)
                                .build(),
                        (billingResult, purchases) -> {
                            // End connection after getting purchases
                            endConnection();

                            PurchaseModel activePurchaseModel = null;
                            if (billingResult.getResponseCode()
                                    == BillingClient.BillingResponseCode.OK) {
                                for (Purchase purchase : purchases) {
                                    if (purchase.getPurchaseState()
                                            == Purchase.PurchaseState.PURCHASED) {
                                        activePurchaseModel = new PurchaseModel(
                                                purchase.getPurchaseToken(),
                                                purchase.getProducts().get(0).toString(), purchase);
                                        break;
                                    }
                                }
                            } else {
                                Log.e(TAG,
                                        "queryPurchases failed" + billingResult.getDebugMessage());
                                BraveVpnUtils.showToast(billingResult.getDebugMessage());
                            }
                            mutableActivePurchases.postValue(activePurchaseModel);
                        });
            } else {
                mutableActivePurchases.postValue(null);
            }
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
        MutableLiveData<Boolean> _billingConnectionState = new MutableLiveData();
        LiveData<Boolean> billingConnectionState = _billingConnectionState;
        startBillingServiceConnection(_billingConnectionState);
        LiveDataUtil.observeOnce(billingConnectionState, isConnected -> {
            if (isConnected) {
                BillingResult billingResult =
                        mBillingClient.launchBillingFlow(activity, billingFlowParams);
            }
        });
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
            MutableLiveData<Boolean> _billingConnectionState = new MutableLiveData();
            LiveData<Boolean> billingConnectionState = _billingConnectionState;
            startBillingServiceConnection(_billingConnectionState);
            LiveDataUtil.observeOnce(billingConnectionState, isConnected -> {
                if (isConnected) {
                    mBillingClient.acknowledgePurchase(acknowledgePurchaseParams, billingResult -> {
                        // End connection after getting the resposne of the purchase aknowledgment
                        endConnection();
                        BraveActivity activity = null;
                        try {
                            activity = BraveActivity.getBraveActivity();
                        } catch (BraveActivity.BraveActivityNotFoundException e) {
                            Log.e(TAG, "acknowledgePurchase " + e.getMessage());
                        }
                        if (billingResult.getResponseCode()
                                == BillingClient.BillingResponseCode.OK) {
                            BraveVpnPrefUtils.setSubscriptionPurchase(true);
                            if (activity != null) {
                                BraveVpnUtils.openBraveVpnProfileActivity(activity);
                                BraveVpnUtils.showToast(activity.getResources().getString(
                                        R.string.subscription_consumed));
                            }
                        } else {
                            BraveVpnUtils.showToast(
                                    context.getResources().getString(R.string.fail_to_aknowledge));
                        }
                    });
                }
            });
        } else {
            BraveVpnPrefUtils.setSubscriptionPurchase(true);
        }
    }

    private PurchasesUpdatedListener getPurchasesUpdatedListener(Context context) {
        return (billingResult, purchases) -> {
            endConnection();
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                if (purchases != null) {
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
                    == BillingClient.BillingResponseCode.USER_CANCELED) {
                BraveVpnUtils.showToast(
                        context.getResources().getString(R.string.error_caused_by_user));
            } else {
                BraveVpnUtils.showToast(
                        context.getResources().getString(R.string.purchased_failed));
            }
        };
    }

    private int maxTries;
    private int tries;
    private boolean isConnectionEstablished;
    private void retryBillingServiceConnection(
            MutableLiveData<Boolean> billingClientConnectionState) {
        maxTries = 3;
        tries = 1;
        isConnectionEstablished = false;
        do {
            try {
                // End existing connection if any before we start another connection
                endConnection();

                Context context = ContextUtils.getApplicationContext();

                mBillingClient = BillingClient.newBuilder(context)
                                         .enablePendingPurchases()
                                         .setListener(getPurchasesUpdatedListener(context))
                                         .build();

                mBillingClient.startConnection(new BillingClientStateListener() {
                    @Override
                    public void onBillingServiceDisconnected() {
                        if (tries == maxTries && billingClientConnectionState != null) {
                            billingClientConnectionState.postValue(false);
                        }
                    }
                    @Override
                    public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                        if (billingResult.getResponseCode()
                                == BillingClient.BillingResponseCode.OK) {
                            isConnectionEstablished = true;
                            if (billingClientConnectionState != null) {
                                billingClientConnectionState.postValue(true);
                            }
                        } else {
                            BraveVpnUtils.showToast(billingResult.getDebugMessage());
                        }
                    }
                });
            } catch (Exception ex) {
                Log.e(TAG, "retryBillingServiceConnection " + ex.getMessage());
            } finally {
                tries++;
            }
        } while (tries <= maxTries && !isConnectionEstablished);
    }

    private ProductDetails.PricingPhase getPricingPhase(ProductDetails productDetails) {
        if (productDetails.getSubscriptionOfferDetails() != null) {
            for (ProductDetails.SubscriptionOfferDetails subscriptionOfferDetails :
                    productDetails.getSubscriptionOfferDetails()) {
                if (subscriptionOfferDetails.getOfferId() == null) {
                    for (ProductDetails.PricingPhase pricingPhase :
                            subscriptionOfferDetails.getPricingPhases().getPricingPhaseList()) {
                        if (pricingPhase.getPriceAmountMicros() > 0) {
                            return pricingPhase;
                        }
                    }
                }
            }
        }
        return null;
    }

    public String getFormattedProductPrice(ProductDetails productDetails) {
        ProductDetails.PricingPhase pricingPhase = getPricingPhase(productDetails);
        if (pricingPhase != null) {
            double price = ((double) pricingPhase.getPriceAmountMicros() / MICRO_UNITS);
            String priceString = String.format(Locale.getDefault(), "%.2f", price);
            return pricingPhase.getPriceCurrencyCode() + " " + priceString;
        }
        return null;
    }

    public String getFormattedFullProductPrice(ProductDetails productDetails) {
        ProductDetails.PricingPhase pricingPhase = getPricingPhase(productDetails);
        if (pricingPhase != null) {
            double yearlyPrice = ((double) pricingPhase.getPriceAmountMicros() / MICRO_UNITS) * 12;
            String priceString = String.format(Locale.getDefault(), "%.2f", yearlyPrice);
            return pricingPhase.getPriceCurrencyCode() + " " + priceString;
        }
        return null;
    }
}

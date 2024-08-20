/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.billing;

import android.app.Activity;
import android.content.Context;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.TextAppearanceSpan;

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
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.brave_leo.BraveLeoUtils;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Currency;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class InAppPurchaseWrapper {
    public static final String MANAGE_SUBSCRIPTION_PAGE =
            "https://play.google.com/store/account/subscriptions";
    private static final String TAG = "InAppPurchaseWrapper";
    private static final String LEO_MONTHLY_SUBSCRIPTION = "brave.leo.monthly";

    private static final String VPN_NIGHTLY_MONTHLY_SUBSCRIPTION = "nightly.bravevpn.monthly";
    private static final String VPN_NIGHTLY_YEARLY_SUBSCRIPTION = "nightly.bravevpn.yearly";

    private static final String VPN_BETA_MONTHLY_SUBSCRIPTION = "beta.bravevpn.monthly";
    private static final String VPN_BETA_YEARLY_SUBSCRIPTION = "beta.bravevpn.yearly";

    public static final String VPN_RELEASE_MONTHLY_SUBSCRIPTION = "brave.vpn.monthly";
    public static final String VPN_RELEASE_YEARLY_SUBSCRIPTION = "brave.vpn.yearly";
    private BillingClient mBillingClient;

    private static final long MICRO_UNITS =
            1000000; // 1,000,000 micro-units equal one unit of the currency

    private static volatile InAppPurchaseWrapper sInAppPurchaseWrapper;
    private static Object sMutex = new Object();

    private enum SubscriptionType {
        MONTHLY,
        YEARLY
    }

    public enum SubscriptionProduct {
        VPN,
        LEO
    }

    private MutableLiveData<ProductDetails> mMutableMonthlyProductDetailsVPN =
            new MutableLiveData();
    private LiveData<ProductDetails> mMonthlyProductDetailsVPN = mMutableMonthlyProductDetailsVPN;
    private MutableLiveData<ProductDetails> mMutableMonthlyProductDetailsLeo =
            new MutableLiveData();
    private LiveData<ProductDetails> mMonthlyProductDetailsLeo = mMutableMonthlyProductDetailsLeo;

    private void setMonthlyProductDetails(
            ProductDetails productDetails, SubscriptionProduct product) {
        if (product.equals(SubscriptionProduct.LEO)) {
            mMutableMonthlyProductDetailsLeo.postValue(productDetails);
        } else if (product.equals(SubscriptionProduct.VPN)) {
            mMutableMonthlyProductDetailsVPN.postValue(productDetails);
        }
    }

    public LiveData<ProductDetails> getMonthlyProductDetails(SubscriptionProduct product) {
        if (product.equals(SubscriptionProduct.LEO)) {
            return mMonthlyProductDetailsLeo;
        }

        return mMonthlyProductDetailsVPN;
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
        return productId.equals(VPN_NIGHTLY_MONTHLY_SUBSCRIPTION)
                || productId.equals(VPN_BETA_MONTHLY_SUBSCRIPTION)
                || productId.equals(VPN_RELEASE_MONTHLY_SUBSCRIPTION);
    }

    private void startBillingServiceConnection(
            MutableLiveData<Boolean> billingClientConnectionState) {
        Context context = ContextUtils.getApplicationContext();
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
                            showToast(billingResult.getDebugMessage());
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

    public String getProductId(SubscriptionProduct product, SubscriptionType subscriptionType) {
        if (product.equals(SubscriptionProduct.VPN)) {
            String bravePackageName = ContextUtils.getApplicationContext().getPackageName();
            if (bravePackageName.equals(BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME)) {
                return subscriptionType == SubscriptionType.MONTHLY ?
                        VPN_RELEASE_MONTHLY_SUBSCRIPTION
                        : VPN_RELEASE_YEARLY_SUBSCRIPTION;
            } else if (bravePackageName.equals(BraveConstants.BRAVE_BETA_PACKAGE_NAME)) {
                return subscriptionType == SubscriptionType.MONTHLY
                        ? VPN_BETA_MONTHLY_SUBSCRIPTION
                        : VPN_BETA_YEARLY_SUBSCRIPTION;
            } else {
                return subscriptionType == SubscriptionType.MONTHLY ?
                        VPN_NIGHTLY_MONTHLY_SUBSCRIPTION
                        : VPN_NIGHTLY_YEARLY_SUBSCRIPTION;
            }
        } else if (product.equals(SubscriptionProduct.LEO)) {
            return LEO_MONTHLY_SUBSCRIPTION;
        } else {
            assert false;
            return "";
        }
    }

    public void queryProductDetailsAsync(SubscriptionProduct product) {
        Map<String, ProductDetails> productDetails = new HashMap<>();
        List<QueryProductDetailsParams.Product> products = new ArrayList<>();
        products.add(QueryProductDetailsParams.Product.newBuilder()
                             .setProductId(getProductId(product, SubscriptionType.MONTHLY))
                             .setProductType(BillingClient.ProductType.SUBS)
                             .build());
        if (!product.equals(SubscriptionProduct.LEO)) {
            products.add(QueryProductDetailsParams.Product.newBuilder()
                    .setProductId(getProductId(product, SubscriptionType.YEARLY))
                    .setProductType(BillingClient.ProductType.SUBS)
                    .build());
        }
        QueryProductDetailsParams queryProductDetailsParams =
                QueryProductDetailsParams.newBuilder().setProductList(products).build();

        MutableLiveData<Boolean> _billingConnectionState = new MutableLiveData();
        LiveData<Boolean> billingConnectionState = _billingConnectionState;
        startBillingServiceConnection(_billingConnectionState);
        LiveDataUtil.observeOnce(
                billingConnectionState,
                isConnected -> {
                    if (isConnected) {
                        mBillingClient.queryProductDetailsAsync(
                                queryProductDetailsParams,
                                (billingResult, productDetailsList) -> {
                                    // End connection after getting the product details
                                    endConnection();

                                    if (billingResult.getResponseCode()
                                            == BillingClient.BillingResponseCode.OK) {
                                        for (ProductDetails productDetail : productDetailsList) {
                                            productDetails.put(
                                                    productDetail.getProductId(), productDetail);
                                        }
                                        setMonthlyProductDetails(
                                                productDetails.get(
                                                        getProductId(
                                                                product, SubscriptionType.MONTHLY)),
                                                product);
                                        if (!product.equals(SubscriptionProduct.LEO)) {
                                            setYearlyProductDetails(
                                                    productDetails.get(
                                                            getProductId(
                                                                    product,
                                                                    SubscriptionType.YEARLY)));
                                        }
                                    } else {
                                        Log.e(
                                                TAG,
                                                "queryProductDetailsAsync failed"
                                                        + billingResult.getDebugMessage());
                                        showToast(billingResult.getDebugMessage());
                                    }
                                });
                    }
                });
    }

    public void queryPurchases(MutableLiveData<PurchaseModel> mutableActivePurchases,
                               SubscriptionProduct type) {
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
                                            != Purchase.PurchaseState.PURCHASED) {
                                        continue;
                                    }
                                    List<String> productIds = purchase.getProducts();
                                    boolean isVPNProduct = isVPNProduct(productIds);
                                    boolean isLeoProduct = isLeoProduct(productIds);
                                    if (isVPNProduct && type.equals(SubscriptionProduct.VPN) ||
                                            isLeoProduct && type.equals(SubscriptionProduct.LEO)) {
                                        activePurchaseModel = new PurchaseModel(
                                                purchase.getPurchaseToken(),
                                                purchase.getProducts().get(0).toString(), purchase);
                                        break;
                                    }
                                }
                            } else {
                                Log.e(TAG,
                                        "queryPurchases failed" + billingResult.getDebugMessage());
                                showToast(billingResult.getDebugMessage());
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
        List<String> productIds = purchase.getProducts();
        boolean isVPNProduct = isVPNProduct(productIds);
        boolean isLeoProduct = isLeoProduct(productIds);
        if (!purchase.isAcknowledged()) {
            MutableLiveData<Boolean> _billingConnectionState = new MutableLiveData();
            LiveData<Boolean> billingConnectionState = _billingConnectionState;
            startBillingServiceConnection(_billingConnectionState);
            LiveDataUtil.observeOnce(
                    billingConnectionState,
                    isConnected -> {
                        if (isConnected) {
                            mBillingClient.acknowledgePurchase(
                                    acknowledgePurchaseParams,
                                    billingResult -> {
                                        // End connection after getting the response of the purchase
                                        // acknowledgment
                                        endConnection();
                                        if (billingResult.getResponseCode()
                                                == BillingClient.BillingResponseCode.OK) {
                                            receiptAcknowledged(
                                                    context, purchase, isVPNProduct, isLeoProduct);
                                        } else {
                                            showToast(
                                                    context.getResources()
                                                            .getString(
                                                                    R.string.fail_to_aknowledge));
                                        }
                                    });
                        }
                    });
        } else {
            if (isVPNProduct) {
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
            } else if (isLeoProduct) {
                receiptAcknowledged(context, purchase, false, true);
            }
        }
    }

    private void receiptAcknowledged(
            Context context, Purchase purchase, boolean isVPNProduct, boolean isLeoProduct) {
        BraveActivity activity = null;
        try {
            activity = BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "acknowledgePurchase " + e.getMessage());
        }
        if (isVPNProduct) {
            BraveVpnPrefUtils.setSubscriptionPurchase(true);
            if (activity != null) {
                BraveVpnUtils.openBraveVpnProfileActivity(activity);
            }
        } else if (isLeoProduct && activity != null) {
            activity.runOnUiThread(
                    new Runnable() {
                        @Override
                        public void run() {
                            BraveLeoPrefUtils.setIsSubscriptionActive(true);
                            BraveLeoPrefUtils.setChatPackageName();
                            BraveLeoPrefUtils.setChatProductId(
                                    purchase.getProducts().get(0).toString());
                            BraveLeoPrefUtils.setChatPurchaseToken(purchase.getPurchaseToken());
                            BraveLeoUtils.bringMainActivityOnTop();
                        }
                    });
        }
        showToast(context.getResources().getString(R.string.subscription_consumed));
    }

    private boolean isVPNProduct(List<String> productIds) {
        return productIds.contains(VPN_NIGHTLY_MONTHLY_SUBSCRIPTION) ||
                productIds.contains(VPN_NIGHTLY_YEARLY_SUBSCRIPTION) ||
                productIds.contains(VPN_BETA_MONTHLY_SUBSCRIPTION) ||
                productIds.contains(VPN_BETA_YEARLY_SUBSCRIPTION) ||
                productIds.contains(VPN_RELEASE_MONTHLY_SUBSCRIPTION) ||
                productIds.contains(VPN_RELEASE_YEARLY_SUBSCRIPTION);
    }

    private boolean isLeoProduct(List<String> productIds) {
        return productIds.contains(LEO_MONTHLY_SUBSCRIPTION);
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
                showToast(
                        context.getResources().getString(R.string.already_subscribed));
            } else if (billingResult.getResponseCode()
                    == BillingClient.BillingResponseCode.USER_CANCELED) {
                showToast(
                        context.getResources().getString(R.string.error_caused_by_user));
            } else {
                showToast(
                        context.getResources().getString(R.string.purchased_failed));
            }
        };
    }

    private int mMaxTries;
    private int mTries;
    private boolean mIsConnectionEstablished;
    private void retryBillingServiceConnection(
            MutableLiveData<Boolean> billingClientConnectionState) {
        mMaxTries = 3;
        mTries = 1;
        mIsConnectionEstablished = false;
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
                        if (mTries == mMaxTries && billingClientConnectionState != null) {
                            billingClientConnectionState.postValue(false);
                        }
                    }
                    @Override
                    public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                        if (billingResult.getResponseCode()
                                == BillingClient.BillingResponseCode.OK) {
                            mIsConnectionEstablished = true;
                            if (billingClientConnectionState != null) {
                                billingClientConnectionState.postValue(true);
                            }
                        } else {
                            showToast(billingResult.getDebugMessage());
                        }
                    }
                });
            } catch (Exception ex) {
                Log.e(TAG, "retryBillingServiceConnection " + ex.getMessage());
            } finally {
                mTries++;
            }
        } while (mTries <= mMaxTries && !mIsConnectionEstablished);
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

    public SpannableString getFormattedProductPrice(
            Context context, ProductDetails productDetails, int stringRes) {
        ProductDetails.PricingPhase pricingPhase = getPricingPhase(productDetails);
        if (pricingPhase != null) {
            double price = ((double) pricingPhase.getPriceAmountMicros() / MICRO_UNITS);
            String priceString = String.format(Locale.getDefault(), "%.2f", price);
            String currencySymbol =
                    Currency.getInstance(pricingPhase.getPriceCurrencyCode()).getSymbol();
            String priceWithSymbol = currencySymbol + priceString;
            String finalPrice =
                    context.getResources()
                            .getString(
                                    stringRes,
                                    pricingPhase.getPriceCurrencyCode() + " " + priceWithSymbol);
            SpannableString priceSpannable = new SpannableString(finalPrice);
            int index = finalPrice.indexOf(priceWithSymbol);
            priceSpannable.setSpan(
                    new TextAppearanceSpan(context, R.style.LargeSemibold),
                    index,
                    index + priceWithSymbol.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            priceSpannable.setSpan(
                    new ForegroundColorSpan(context.getColor(android.R.color.white)),
                    index,
                    index + priceWithSymbol.length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            return priceSpannable;
        }
        return null;
    }

    public SpannableString getFormattedFullProductPrice(
            Context context, ProductDetails productDetails) {
        ProductDetails.PricingPhase pricingPhase = getPricingPhase(productDetails);
        if (pricingPhase != null) {
            double yearlyPrice = ((double) pricingPhase.getPriceAmountMicros() / MICRO_UNITS) * 12;
            String priceString = String.format(Locale.getDefault(), "%.2f", yearlyPrice);
            String currencySymbol =
                    Currency.getInstance(pricingPhase.getPriceCurrencyCode()).getSymbol();
            String priceWithSymbol = currencySymbol + priceString;
            String finalPrice = pricingPhase.getPriceCurrencyCode() + " " + priceWithSymbol;
            SpannableString priceSpannable = new SpannableString(finalPrice);
            int index = finalPrice.indexOf(priceWithSymbol);
            priceSpannable.setSpan(
                    new TextAppearanceSpan(context, R.style.LargeSemibold),
                    index,
                    index + priceWithSymbol.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            priceSpannable.setSpan(
                    new ForegroundColorSpan(context.getColor(android.R.color.white)),
                    index,
                    index + priceWithSymbol.length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            return priceSpannable;
        }
        return null;
    }

    public int getYearlyDiscountPercentage(
            ProductDetails monthlyProductDetails, ProductDetails yearlyProductDetails) {
        ProductDetails.PricingPhase monthlyPricingPhase = getPricingPhase(monthlyProductDetails);
        ProductDetails.PricingPhase yearlyPricingPhase = getPricingPhase(yearlyProductDetails);
        if (monthlyPricingPhase != null && yearlyPricingPhase != null) {
            double yearlyPrice =
                    ((double) monthlyPricingPhase.getPriceAmountMicros() / MICRO_UNITS) * 12;
            double yearlyActualPrice =
                    ((double) yearlyPricingPhase.getPriceAmountMicros() / MICRO_UNITS);
            if (yearlyPrice == 0) {
                return 0;
            }
            double discountPrice = yearlyPrice - yearlyActualPrice;
            double discountPercentage = (discountPrice / yearlyPrice) * 100;
            return (int) Math.round(discountPercentage);
        }
        return 0;
    }

    private void showToast(String message) {
        Context context = ContextUtils.getApplicationContext();
        Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
    }
}

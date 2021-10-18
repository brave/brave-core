/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.activities;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import com.android.billingclient.api.Purchase;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnProfileCredentials;
import org.chromium.chrome.browser.vpn.utils.BraveVpnApiResponseUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.utils.InAppPurchaseWrapper;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    public boolean mIsVerification;
    private BraveVpnPrefModel mBraveVpnPrefModel;

    abstract void showRestoreMenu(boolean shouldShowRestore);
    abstract void showProgress();
    abstract void hideProgress();

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(
                    BraveVpnParentActivity.this);
        }
    }

    protected void verifySubscription() {
        showProgress();
        mBraveVpnPrefModel = new BraveVpnPrefModel();
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases != null && purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mBraveVpnPrefModel.setPurchaseToken(purchase.getPurchaseToken());
            mBraveVpnPrefModel.setProductId(purchase.getSkus().get(0).toString());
            if (mIsVerification) {
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        mBraveVpnPrefModel.getPurchaseToken(), mBraveVpnPrefModel.getProductId(),
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
            } else {
                BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, mBraveVpnPrefModel.getProductId(),
                        BraveVpnUtils.IAP_ANDROID_PARAM_TEXT, mBraveVpnPrefModel.getPurchaseToken(),
                        getPackageName());
            }
        } else {
            if (!mIsVerification) {
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
            } else {
                hideProgress();
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess && mBraveVpnPrefModel != null) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(mBraveVpnPrefModel.getPurchaseToken());
                BraveVpnPrefUtils.setProductId(mBraveVpnPrefModel.getProductId());
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
                if (mIsVerification) {
                    mIsVerification = false;
                    showRestoreMenu(true);
                    Toast.makeText(BraveVpnParentActivity.this, R.string.already_subscribed,
                                 Toast.LENGTH_SHORT)
                            .show();
                    hideProgress();
                } else {
                    BraveVpnProfileUtils.getInstance().startStopVpn(BraveVpnParentActivity.this);
                }
            } else {
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
                if (mIsVerification) {
                    mIsVerification = false;
                    showRestoreMenu(false);
                    hideProgress();
                } else {
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
                }
            }
        }
    };

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        mBraveVpnPrefModel.setSubscriberCredential(subscriberCredential);
        BraveVpnApiResponseUtils.handleOnGetSubscriberCredential(
                BraveVpnParentActivity.this, isSuccess);
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        BraveVpnApiResponseUtils.handleOnGetTimezonesForRegions(
                BraveVpnParentActivity.this, jsonTimezones, isSuccess);
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        mBraveVpnPrefModel.setHostname(BraveVpnApiResponseUtils.handleOnGetHostnamesForRegion(
                BraveVpnParentActivity.this, mBraveVpnPrefModel, jsonHostNames, isSuccess));
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        BraveVpnApiResponseUtils.handleOnGetProfileCredentials(
                BraveVpnParentActivity.this, mBraveVpnPrefModel, jsonProfileCredentials, isSuccess);
    }
}

/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.util.Pair;

import com.android.billingclient.api.Purchase;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    public boolean mIsVerification;
    private String mSubscriberCredential = "";
    private String mHostname = "";
    private String mPurchaseToken = "";
    private String mProductId = "";

    abstract void showRestoreMenu(boolean shouldShowRestore);
    abstract void showProgress();
    abstract void hideProgress();

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        InAppPurchaseWrapper.getInstance().startBillingServiceConnection(
                BraveVpnParentActivity.this);
    }

    protected void verifySubscription() {
        showProgress();
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mPurchaseToken = purchase.getPurchaseToken();
            mProductId = purchase.getSkus().get(0).toString();
            if (mIsVerification) {
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(mPurchaseToken, mProductId,
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
            } else {
                BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, mProductId,
                        BraveVpnUtils.IAP_ANDROID_PARAM_TEXT, mPurchaseToken, getPackageName());
            }
        } else {
            if (!mIsVerification) {
                BraveVpnPrefUtils.setPurchaseToken("");
                BraveVpnPrefUtils.setProductId("");
                BraveVpnPrefUtils.setPurchaseExpiry(0L);
                BraveVpnPrefUtils.setSubscriptionPurchase(false);
                if (BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this)
                                .isVPNConnected()) {
                    BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this).stopVpn();
                }
                BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this).deleteVpnProfile();
                BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
            } else {
                hideProgress();
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
                BraveVpnPrefUtils.setProductId(mProductId);
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
                    BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this).startStopVpn();
                }
            } else {
                BraveVpnPrefUtils.setPurchaseToken("");
                BraveVpnPrefUtils.setProductId("");
                BraveVpnPrefUtils.setPurchaseExpiry(0L);
                BraveVpnPrefUtils.setSubscriptionPurchase(false);
                if (BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this)
                                .isVPNConnected()) {
                    BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this).stopVpn();
                }
                BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this).deleteVpnProfile();
                Toast.makeText(BraveVpnParentActivity.this,
                             R.string.purchase_token_verification_failed, Toast.LENGTH_SHORT)
                        .show();
                if (mIsVerification) {
                    mIsVerification = false;
                    showRestoreMenu(false);
                    hideProgress();
                } else {
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
                }
            }
            mPurchaseToken = "";
            mProductId = "";
        }
    };

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        if (isSuccess) {
            InAppPurchaseWrapper.getInstance().processPurchases(
                    InAppPurchaseWrapper.getInstance().queryPurchases());
            this.mSubscriberCredential = subscriberCredential;
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_SHORT)
                    .show();
            Log.e("BraveVPN", "BraveVpnParentActivity -> onGetSubscriberCredential : failed");
            hideProgress();
        }
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID());
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(region);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_SHORT)
                    .show();
            Log.e("BraveVPN", "BraveVpnParentActivity -> onGetTimezonesForRegions : failed");
            hideProgress();
        }
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        if (isSuccess) {
            mHostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            BraveVpnNativeWorker.getInstance().getProfileCredentials(
                    mSubscriberCredential, mHostname);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_SHORT)
                    .show();
            Log.e("BraveVPN", "BraveVpnParentActivity -> onGetHostnamesForRegion : failed");
            hideProgress();
        }
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        if (isSuccess) {
            Pair<String, String> profileCredentials =
                    BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            BraveVpnPrefUtils.setHostname(mHostname);
            BraveVpnProfileUtils.getInstance(BraveVpnParentActivity.this)
                    .createVpnProfile(BraveVpnParentActivity.this, mHostname,
                            profileCredentials.first, profileCredentials.second);
            BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
            BraveVpnPrefUtils.setProductId(mProductId);
            mPurchaseToken = "";
            mProductId = "";
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_SHORT)
                    .show();
            Log.e("BraveVPN", "BraveVpnParentActivity -> jsonProfileCredentials : failed");
            hideProgress();
        }
    }
}

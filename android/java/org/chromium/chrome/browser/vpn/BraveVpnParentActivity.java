/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import com.android.billingclient.api.Purchase;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnProfileCredentials;
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
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(
                    BraveVpnParentActivity.this);
        }
    }

    protected void verifySubscription() {
        showProgress();
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases != null && purchases.size() == 1) {
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
                if (BraveVpnProfileUtils.getInstance().isVPNConnected(
                            BraveVpnParentActivity.this)) {
                    BraveVpnProfileUtils.getInstance().stopVpn(BraveVpnParentActivity.this);
                }
                BraveVpnProfileUtils.getInstance().deleteVpnProfile(BraveVpnParentActivity.this);
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
                    BraveVpnProfileUtils.getInstance().startStopVpn(BraveVpnParentActivity.this);
                }
            } else {
                BraveVpnPrefUtils.setPurchaseToken("");
                BraveVpnPrefUtils.setProductId("");
                BraveVpnPrefUtils.setPurchaseExpiry(0L);
                BraveVpnPrefUtils.setSubscriptionPurchase(false);
                if (BraveVpnProfileUtils.getInstance().isVPNConnected(
                            BraveVpnParentActivity.this)) {
                    BraveVpnProfileUtils.getInstance().stopVpn(BraveVpnParentActivity.this);
                }
                BraveVpnProfileUtils.getInstance().deleteVpnProfile(BraveVpnParentActivity.this);
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
            InAppPurchaseWrapper.getInstance().processPurchases(BraveVpnParentActivity.this,
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
            String serverRegion = BraveVpnPrefUtils.getServerRegion();
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(
                    serverRegion.equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC) ? region
                                                                                    : serverRegion);
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
            BraveVpnProfileCredentials braveVpnProfileCredentials =
                    BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            BraveVpnPrefUtils.setHostname(mHostname);
            if (BraveVpnProfileUtils.getInstance().isVPNConnected(BraveVpnParentActivity.this)) {
                BraveVpnProfileUtils.getInstance().stopVpn(BraveVpnParentActivity.this);
            }
            try {
                BraveVpnProfileUtils.getInstance().createVpnProfile(BraveVpnParentActivity.this,
                        mHostname, braveVpnProfileCredentials.getUsername(),
                        braveVpnProfileCredentials.getPassword());
                BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
                BraveVpnPrefUtils.setProductId(mProductId);
                BraveVpnPrefUtils.setSubscriberCredential(mSubscriberCredential);
            } catch (Exception securityException) {
                BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
                finish();
            }
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

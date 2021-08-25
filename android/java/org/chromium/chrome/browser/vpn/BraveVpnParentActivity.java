/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.content.Context;
import android.content.Intent;
import android.graphics.Paint;
import android.net.VpnManager;
import android.os.Bundle;
import android.util.Pair;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.viewpager.widget.ViewPager;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnConfirmDialogFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    public boolean isVerification = false;
    private String subscriberCredential = "";
    private String hostname = "";
    private String purchaseToken = "";
    private String productId = "";

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
        Log.e("BraveVPN", "BraveVpnParentActivity :  purchases.size() : " + purchases.size());
        if (purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            purchaseToken = purchase.getPurchaseToken();
            productId = purchase.getSkus().get(0).toString();
            Log.e("BraveVPN", "Purchase Token : " + purchaseToken);
            if (isVerification) {
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        purchaseToken, productId, "subscription", getPackageName());
            } else {
                BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                        "subscription", productId, "iap-android", purchaseToken, getPackageName());
            }
        } else {
            if (!isVerification) {
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, "");
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, "");
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY, "");
                BraveVpnPrefUtils.setBraveVpnBooleanPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false);
                if (VpnProfileUtils.getInstance(BraveVpnParentActivity.this).isVPNConnected()) {
                    VpnProfileUtils.getInstance(BraveVpnParentActivity.this).stopVpn();
                }
                VpnProfileUtils.getInstance(BraveVpnParentActivity.this).deleteVpnProfile();
                BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
            } else {
                Toast.makeText(BraveVpnParentActivity.this,
                             R.string.purchase_token_verification_failed, Toast.LENGTH_LONG)
                        .show();
                hideProgress();
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            String purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (!purchaseExpiry.isEmpty()
                    && Long.parseLong(purchaseExpiry) >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, purchaseToken);
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, productId);
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY, purchaseExpiry);
                BraveVpnPrefUtils.setBraveVpnBooleanPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, true);
                if (isVerification) {
                    isVerification = false;
                    showRestoreMenu(true);
                    hideProgress();
                } else {
                    VpnProfileUtils.getInstance(BraveVpnParentActivity.this).startStopVpn();
                }
            } else {
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, "");
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, "");
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY, "");
                BraveVpnPrefUtils.setBraveVpnBooleanPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false);
                if (VpnProfileUtils.getInstance(BraveVpnParentActivity.this).isVPNConnected()) {
                    VpnProfileUtils.getInstance(BraveVpnParentActivity.this).stopVpn();
                }
                VpnProfileUtils.getInstance(BraveVpnParentActivity.this).deleteVpnProfile();
                Toast.makeText(BraveVpnParentActivity.this,
                             R.string.purchase_token_verification_failed, Toast.LENGTH_LONG)
                        .show();
                if (isVerification) {
                    isVerification = false;
                    showRestoreMenu(false);
                    hideProgress();
                } else {
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
                }
            }
            purchaseToken = "";
            productId = "";
        }
    };

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        Log.e("BraveVPN", "isSuccess : " + isSuccess);
        if (isSuccess) {
            InAppPurchaseWrapper.getInstance().processPurchases(
                    InAppPurchaseWrapper.getInstance().queryPurchases());
            this.subscriberCredential = subscriberCredential;
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "onGetSubscriberCredential : failed");
            hideProgress();
        }
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        Log.e("BraveVPN", jsonTimezones);
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID()); //
            Log.e("BraveVPN", "Region : " + region);
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(region);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "onGetTimezonesForRegions : failed");
            hideProgress();
        }
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        Log.e("BraveVPN", jsonHostNames);
        if (isSuccess) {
            hostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            Log.e("BraveVPN", "Hostname : " + hostname);
            BraveVpnNativeWorker.getInstance().getProfileCredentials(
                    subscriberCredential, hostname);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();

            Log.e("BraveVPN", "onGetHostnamesForRegion : failed");
            hideProgress();
        }
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        Log.e("BraveVPN", jsonProfileCredentials);
        if (isSuccess) {
            Pair<String, String> profileCredentials =
                    BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_HOSTNAME, hostname);
            VpnProfileUtils.getInstance(BraveVpnParentActivity.this)
                    .createVpnProfile(BraveVpnParentActivity.this, hostname,
                            profileCredentials.first, profileCredentials.second);
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, purchaseToken);
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, productId);
            purchaseToken = "";
            productId = "";
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "jsonProfileCredentials : failed");
            hideProgress();
        }
    }
}

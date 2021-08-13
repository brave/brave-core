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
import android.util.Pair;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnConfirmDialogFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    private String subscriberCredential;
    private String hostname;

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        InAppPurchaseWrapper.getInstance().startBillingServiceConnection(BraveVpnParentActivity.this);
    }

    protected void getPurchaseDetails() {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        boolean isSubscriptionPurchased = false;
        for (Purchase purchase : purchases) {
            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                String purchaseToken = purchase.getPurchaseToken();
                String productId = purchase.getSkus().get(0).toString();
                Log.e("BraveVPN", "Purchase Token : " + purchaseToken);
                isSubscriptionPurchased = true;
                BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                        "subscription", productId, "iap-android", purchaseToken, getPackageName());
            }
        }

        if (!isSubscriptionPurchased) {
            BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
            Log.e("BraveVPN", "getPurchaseDetails failed");
        }
    }

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        Log.e("BraveVPN", "isSuccess : "+isSuccess);
        if (isSuccess) {
            InAppPurchaseWrapper.getInstance().processPurchases(InAppPurchaseWrapper.getInstance().queryPurchases());
            this.subscriberCredential = subscriberCredential;
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "onGetSubscriberCredential : failed");
        }
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        Log.e("BraveVPN", jsonTimezones);
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID()); //
            Log.e("BraveVPN",
                    "Region : "
                            + region);
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(region);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "onGetTimezonesForRegions : failed");
        }
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        Log.e("BraveVPN", jsonHostNames);
        if (isSuccess) {
            hostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            Log.e("BraveVPN", "Hostname : "+hostname);
            BraveVpnNativeWorker.getInstance().getProfileCredentials(subscriberCredential, hostname);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();

            Log.e("BraveVPN", "onGetHostnamesForRegion : failed");
        }
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        Log.e("BraveVPN", jsonProfileCredentials);
        if (isSuccess) {
            Pair<String,String> profileCredentials = BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            BraveVpnUtils.createVpnProfile(BraveVpnParentActivity.this, hostname, profileCredentials.first, profileCredentials.second);
        } else {
            Toast.makeText(BraveVpnParentActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();

            Log.e("BraveVPN", "jsonProfileCredentials : failed");
        }
    }
}

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

public class BraveVpnProfileActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private TextView profileTitle;
    private TextView profileText;
    private Button installVpnButton;
    private Button contactSupportButton;

    private String subscriberCredential;

    private void initializeViews() {
        setContentView(R.layout.activity_brave_vpn_profile);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.install_vpn));

        profileTitle = findViewById(R.id.brave_vpn_profile_title);
        profileText = findViewById(R.id.brave_vpn_profile_text);

        installVpnButton = findViewById(R.id.btn_install_profile);
        installVpnButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // BraveVpnUtils.startStopVpn(BraveVpnProfileActivity.this);
                if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
                    VpnManager vpnManager =
                            (VpnManager) getSystemService(Context.VPN_MANAGEMENT_SERVICE);
                    if (!BraveVpnUtils.isVPNConnected(BraveVpnProfileActivity.this)
                            && vpnManager != null) {
                        try {
                            vpnManager.startProvisionedVpnProfile();
                        } catch (SecurityException securityException) {
                            // BraveVpnUtils.createVpnProfile(vpnManager, activity);
                            getSubscriptionDetail();
                        }
                    } else {
                        vpnManager.stopProvisionedVpnProfile();
                    }
                }
            }
        });
        installVpnButton.setEnabled(false);

        contactSupportButton = findViewById(R.id.btn_contact_supoort);
        contactSupportButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {}
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void triggerLayoutInflation() {
        mFirstRunFlowSequencer = new FirstRunFlowSequencer(this) {
            @Override
            public void onFlowIsKnown(Bundle freProperties) {
                initializeViews();
            }
        };
        mFirstRunFlowSequencer.start();
        onInitialLayoutInflationComplete();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK && requestCode == BraveVpnUtils.BRAVE_VPN_PROFILE_REQUEST_CODE
                && BraveVpnUtils.isBraveVpnFeatureEnable()) {
            VpnManager vpnManager = (VpnManager) getSystemService(Context.VPN_MANAGEMENT_SERVICE);
            if (vpnManager != null) {
                vpnManager.startProvisionedVpnProfile();
                BraveVpnConfirmDialogFragment braveVpnConfirmDialogFragment =
                        new BraveVpnConfirmDialogFragment();
                braveVpnConfirmDialogFragment.setCancelable(false);
                braveVpnConfirmDialogFragment.show(
                        getSupportFragmentManager(), "BraveVpnConfirmDialogFragment");
            }
            finish();
        } else if (resultCode == RESULT_CANCELED) {
            profileTitle.setText(getResources().getString(R.string.some_context));
            profileText.setText(getResources().getString(R.string.some_context_text));
            installVpnButton.setText(getResources().getString(R.string.accept_connection_request));
            contactSupportButton.setVisibility(View.GONE);
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        BraveVpnNativeWorker.getInstance().addObserver(this);
        installVpnButton.setEnabled(true);
    }

    @Override
    protected void onDestroy() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onDestroy();
    }

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        // Log.e("BraveVPN", jsonTimezones);
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID());
            Log.e("BraveVPN",
                    "Region : "
                            + BraveVpnUtils.getRegionForTimeZone(
                                    jsonTimezones, TimeZone.getDefault().getID()));
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(region);
        } else {
            Toast.makeText(BraveVpnProfileActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
        }
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        Log.e("BraveVPN", jsonHostNames);
        if (isSuccess) {
            String hostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);

        } else {
            Toast.makeText(BraveVpnProfileActivity.this, R.string.vpn_profile_creation_failed,
                         Toast.LENGTH_LONG)
                    .show();
        }
    }

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        if (isSuccess) {
            this.subscriberCredential = subscriberCredential;
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        }
    };

    private void getSubscriptionDetail() {
        BillingClient billingClient = BillingClient.newBuilder(this)
                                              .setListener(purchasesUpdatedListener)
                                              .enablePendingPurchases()
                                              .build();

        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(BillingResult billingResult) {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    billingClient.queryPurchasesAsync(SUBS, (billingResult1, list) -> {
                        if (list.size() > 0) {
                            try {
                                showPurchaseDetails(list);
                            } catch (JSONException e) {
                                e.printStackTrace();
                            }
                        }
                    });
                }
            }
            @Override
            public void onBillingServiceDisconnected() {
                // Try to restart the connection on the next request to
                // Google Play by calling the startConnection() method.
                Log.e("BraveVPN", "onBillingServiceDisconnected");
            }
        });
    }

    private void showPurchaseDetails(List<Purchase> purchases) throws JSONException {
        Purchase purchase = purchases.get(0);
        String purchaseToken = purchase.getPurchaseToken();
        String productId = purchase.getSkus().get(0).toString();
        Log.e("BraveVPN", "Purchase Token : " + purchaseToken);
        BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                "subscription", productId, "iap-android", purchaseToken);
    }

    private PurchasesUpdatedListener purchasesUpdatedListener = (billingResult, purchases) -> {
        if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
            if (purchases != null) {
                // try {
                // showPurchaseDetails(purchases);
                // } catch (JSONException e) {
                //     e.printStackTrace();
                // }
            }
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            // TODO query purchase
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.SERVICE_DISCONNECTED) {
            // TODO connect to service again
        } else if (billingResult.getResponseCode()
                == BillingClient.BillingResponseCode.USER_CANCELED) {
            Toast.makeText(this, "ERROR!!\nCaused by a user cancelling the purchase flow.",
                         Toast.LENGTH_SHORT)
                    .show();
        } else {
            Toast.makeText(this, "ERROR!!\nPurchased failed..", Toast.LENGTH_SHORT).show();
        }
    };
}

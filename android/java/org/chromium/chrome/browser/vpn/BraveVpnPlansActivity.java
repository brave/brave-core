/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.graphics.Paint;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.viewpager.widget.ViewPager;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;
import com.android.billingclient.api.SkuDetailsResponseListener;
import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class BraveVpnPlansActivity extends AsyncInitializationActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;

    private void initializeViews() {
        setContentView(R.layout.activity_brave_vpn_plan);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.brave_vpn));

        ViewPager braveRewardsViewPager = findViewById(R.id.brave_rewards_view_pager);

        BraveVpnPlanPagerAdapter braveVpnPlanPagerAdapter = new BraveVpnPlanPagerAdapter(this);
        braveRewardsViewPager.setAdapter(braveVpnPlanPagerAdapter);
        TabLayout braveRewardsTabLayout = findViewById(R.id.brave_rewards_tab_layout);
        braveRewardsTabLayout.setupWithViewPager(braveRewardsViewPager, true);

        TextView removedValueText = findViewById(R.id.removed_value_tv);
        removedValueText.setPaintFlags(
                removedValueText.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);

        LinearLayout monthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        monthlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showInAppDialogForSubcription(BraveVpnUtils.MONTHLY_SUBSCRIPTION);
            }
        });

        LinearLayout yearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        yearlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showInAppDialogForSubcription(BraveVpnUtils.YEARLY_SUBSCRIPTION);
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_brave_vpn, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        } else if (item.getItemId() == R.id.restore) {
            // Do nothing for now
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

    private void showInAppDialogForSubcription(int subscriptionTYpe) {
        BillingClient billingClient = BillingClient.newBuilder(this)
                                              .setListener(purchasesUpdatedListener)
                                              .enablePendingPurchases()
                                              .build();

        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(BillingResult billingResult) {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    Log.e("BraveVPN", "Conection is established");
                    List<String> skuList = new ArrayList<>();
                    skuList.add("nightly.bravevpn.monthly");
                    SkuDetailsParams.Builder params = SkuDetailsParams.newBuilder();
                    params.setSkusList(skuList).setType(SUBS);
                    billingClient.querySkuDetailsAsync(
                            params.build(), new SkuDetailsResponseListener() {
                                @Override
                                public void onSkuDetailsResponse(
                                        @NonNull BillingResult billingResult,
                                        List<SkuDetails> skuDetailsList) {
                                    if (skuDetailsList.size() > 0) {
                                        BillingFlowParams billingFlowParams =
                                                BillingFlowParams.newBuilder()
                                                        .setSkuDetails(skuDetailsList.get(0))
                                                        .build();
                                        int responseCode =
                                                billingClient
                                                        .launchBillingFlow(
                                                                BraveVpnPlansActivity.this,
                                                                billingFlowParams)
                                                        .getResponseCode();
                                    } else {
                                        Toast.makeText(BraveVpnPlansActivity.this,
                                                     "ERROR !!!\nCan't show billing flow.",
                                                     Toast.LENGTH_SHORT)
                                                .show();
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

    private PurchasesUpdatedListener purchasesUpdatedListener = (billingResult, purchases) -> {
        if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
            if (purchases != null) {
                try {
                    showPurchaseDetails(purchases);
                } catch (JSONException e) {
                    e.printStackTrace();
                }
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

    private void showPurchaseDetails(List<Purchase> purchases) throws JSONException {
        Purchase purchase = purchases.get(0);

        Date date = new Date(purchase.getPurchaseTime());
        String fDate = DateFormat.getDateTimeInstance().format(date);
        String details = "Purchase Token : " + purchase.getPurchaseToken()
                + "\n\n\nSku Name : " + purchase.getSkus().toString() + "\n\n\nOriginal Json: "
                + new JSONObject(purchase.getOriginalJson()).toString()
                // + "\n\nOriginal Json : " + new
                // GsonBuilder().setPrettyPrinting().create().toJson(JsonParser.parseString(purchase.getOriginalJson()))
                + "\n\n\nPurchase State : "
                + (purchase.getPurchaseState() == 1 ? "PURCHASED"
                                                    : "PENDING or UNSPECIFIED PURCHASE STATE")
                + "\n\n\nPurchase Time : " + fDate;
        Log.e("BraveVPN", details);
        if (purchase.getPurchaseState() == 1) {
            BraveVpnUtils.openBraveVpnProfileActivity(BraveVpnPlansActivity.this);
            finish();
        }
    }
}

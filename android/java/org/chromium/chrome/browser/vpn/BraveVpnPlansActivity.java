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
import org.chromium.chrome.browser.vpn.BraveVpnParentActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.InAppPurchaseWrapper;
import org.chromium.ui.widget.Toast;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class BraveVpnPlansActivity extends BraveVpnParentActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private static final String NIGHTLY_MONTHLY_SUBSCRIPTION = "nightly.bravevpn.monthly";
    private static final String NIGHTLY_YEARLY_SUBSCRIPTION = "nightly.bravevpn.yearly";

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveVpnNativeWorker.getInstance().addObserver(this);
    }

    @Override
    public void onPauseWithNative() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onPauseWithNative();
    }

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
                InAppPurchaseWrapper.getInstance().purchase(BraveVpnPlansActivity.this,
                        InAppPurchaseWrapper.getInstance().getSkuDetails(
                                NIGHTLY_MONTHLY_SUBSCRIPTION));
            }
        });

        LinearLayout yearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        yearlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                InAppPurchaseWrapper.getInstance().purchase(BraveVpnPlansActivity.this,
                        InAppPurchaseWrapper.getInstance().getSkuDetails(
                                NIGHTLY_YEARLY_SUBSCRIPTION));
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
            verifySubscription(true);
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
}

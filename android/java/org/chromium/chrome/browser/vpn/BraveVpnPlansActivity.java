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
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
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
    private ProgressBar planProgress;
    private LinearLayout planLayout;
    private boolean shouldShowRestoreMenu;

    private LinearLayout monthlySelectorLayout;
    private LinearLayout yearlySelectorLayout;

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

        planProgress = findViewById(R.id.plan_progress);
        planLayout = findViewById(R.id.plan_layout);

        ViewPager braveRewardsViewPager = findViewById(R.id.brave_rewards_view_pager);

        BraveVpnPlanPagerAdapter braveVpnPlanPagerAdapter = new BraveVpnPlanPagerAdapter(this);
        braveRewardsViewPager.setAdapter(braveVpnPlanPagerAdapter);
        TabLayout braveRewardsTabLayout = findViewById(R.id.brave_rewards_tab_layout);
        braveRewardsTabLayout.setupWithViewPager(braveRewardsViewPager, true);

        TextView removedValueText = findViewById(R.id.removed_value_tv);
        removedValueText.setPaintFlags(
                removedValueText.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);

        SkuDetails monthlySkuDetails = InAppPurchaseWrapper.getInstance().getSkuDetails(
                InAppPurchaseWrapper.NIGHTLY_MONTHLY_SUBSCRIPTION);

        monthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        monthlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showProgress();
                InAppPurchaseWrapper.getInstance().purchase(
                        BraveVpnPlansActivity.this, monthlySkuDetails);
            }
        });

        TextView monthlySubscriptionAmountText =
                findViewById(R.id.monthly_subscription_amount_text);
        monthlySubscriptionAmountText.setText(
                String.format(getResources().getString(R.string.monthly_subscription_amount),
                        monthlySkuDetails.getPrice()));

        SkuDetails yearlySkuDetails = InAppPurchaseWrapper.getInstance().getSkuDetails(
                InAppPurchaseWrapper.NIGHTLY_YEARLY_SUBSCRIPTION);

        yearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        yearlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showProgress();
                InAppPurchaseWrapper.getInstance().purchase(
                        BraveVpnPlansActivity.this, yearlySkuDetails);
            }
        });

        TextView yearlySubscriptionAmountText = findViewById(R.id.yearly_subscription_amount_text);
        yearlySubscriptionAmountText.setText(
                String.format(getResources().getString(R.string.yearly_subscription_amount),
                        yearlySkuDetails.getPrice()));
        isVerification = true;
        verifySubscription();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_brave_vpn, menu);
        MenuItem item = menu.findItem(R.id.restore);
        if (shouldShowRestoreMenu) {
            shouldShowRestoreMenu = false;
            item.setVisible(true);
            if (monthlySelectorLayout != null) {
                monthlySelectorLayout.setAlpha(0.4f);
                monthlySelectorLayout.setOnClickListener(null);
            }

            if (yearlySelectorLayout != null) {
                yearlySelectorLayout.setAlpha(0.4f);
                yearlySelectorLayout.setOnClickListener(null);
            }
        } else {
            item.setVisible(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        } else if (item.getItemId() == R.id.restore) {
            // showProgress();
            // verifySubscription(true);
            BraveVpnUtils.openBraveVpnProfileActivity(BraveVpnPlansActivity.this);
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

    public void showRestoreMenu(boolean shouldShowRestore) {
        this.shouldShowRestoreMenu = shouldShowRestore;
        invalidateOptionsMenu();
    }

    public void showProgress() {
        if (planProgress != null) {
            planProgress.setVisibility(View.VISIBLE);
        }
        if (planLayout != null) {
            planLayout.setAlpha(0.4f);
        }
    }

    public void hideProgress() {
        if (planProgress != null) {
            planProgress.setVisibility(View.GONE);
        }
        if (planLayout != null) {
            planLayout.setAlpha(1f);
        }
    }
}

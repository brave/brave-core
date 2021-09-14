/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.graphics.Paint;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.viewpager.widget.ViewPager;

import com.android.billingclient.api.SkuDetails;
import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.vpn.BraveVpnParentActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.InAppPurchaseWrapper;

public class BraveVpnPlansActivity extends BraveVpnParentActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private ProgressBar mPlanProgress;
    private LinearLayout mPlanLayout;
    private boolean mShouldShowRestoreMenu;

    private LinearLayout mMonthlySelectorLayout;
    private LinearLayout mYearlySelectorLayout;

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveVpnNativeWorker.getInstance().addObserver(this);
        hideProgress();
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

        mPlanProgress = findViewById(R.id.plan_progress);
        mPlanLayout = findViewById(R.id.plan_layout);

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

        mMonthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        mMonthlySelectorLayout.setOnClickListener(new View.OnClickListener() {
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

        mYearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        mYearlySelectorLayout.setOnClickListener(new View.OnClickListener() {
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
        mIsVerification = true;
        verifySubscription();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_brave_vpn, menu);
        MenuItem item = menu.findItem(R.id.restore);
        if (mShouldShowRestoreMenu) {
            mShouldShowRestoreMenu = false;
            item.setVisible(true);
            if (mMonthlySelectorLayout != null) {
                mMonthlySelectorLayout.setAlpha(0.4f);
                mMonthlySelectorLayout.setOnClickListener(null);
            }

            if (mYearlySelectorLayout != null) {
                mYearlySelectorLayout.setAlpha(0.4f);
                mYearlySelectorLayout.setOnClickListener(null);
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

    @Override
    public void showRestoreMenu(boolean shouldShowRestore) {
        this.mShouldShowRestoreMenu = shouldShowRestore;
        invalidateOptionsMenu();
    }

    @Override
    public void showProgress() {
        if (mPlanProgress != null) {
            mPlanProgress.setVisibility(View.VISIBLE);
        }
        if (mPlanLayout != null) {
            mPlanLayout.setAlpha(0.4f);
        }
    }

    @Override
    public void hideProgress() {
        if (mPlanProgress != null) {
            mPlanProgress.setVisibility(View.GONE);
        }
        if (mPlanLayout != null) {
            mPlanLayout.setAlpha(1f);
        }
    }
}

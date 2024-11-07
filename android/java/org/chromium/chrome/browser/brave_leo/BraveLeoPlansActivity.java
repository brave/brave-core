/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.text.SpannableString;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import com.android.billingclient.api.ProductDetails;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

/** Brave's Activity for AI Chat Plans */
public class BraveLeoPlansActivity extends AsyncInitializationActivity {
    private ProgressBar mMonthlyPlanProgress;
    private ProgressBar mYearlyPlanProgress;
    private TextView mMonthlySubscriptionAmountText;
    private TextView mYearlySubscriptionAmountText;
    private TextView mUpgradeButton;
    private InAppPurchaseWrapper.SubscriptionType mCurrentSelectedPlan =
            InAppPurchaseWrapper.SubscriptionType.YEARLY;
    private LinearLayout mMonthlySelectorLayout;
    private LinearLayout mYearlySelectorLayout;
    ProductDetails mMonthlyProductDetails;
    ProductDetails mYearlyProductDetails;

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_brave_leo_plans);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.brave_leo_premium));

        mMonthlyPlanProgress = findViewById(R.id.monthly_plan_progress);
        mMonthlySubscriptionAmountText = findViewById(R.id.monthly_subscription_amount_text);
        mUpgradeButton = findViewById(R.id.tv_upgrade_now);
        mYearlyPlanProgress = findViewById(R.id.yearly_plan_progress);
        mYearlySubscriptionAmountText = findViewById(R.id.yearly_subscription_amount_text);
        mMonthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        mYearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        mMonthlySelectorLayout.setOnClickListener(
                v -> {
                    mCurrentSelectedPlan = InAppPurchaseWrapper.SubscriptionType.MONTHLY;
                    updateSelectedPlanView();
                });
        mYearlySelectorLayout.setOnClickListener(
                v -> {
                    mCurrentSelectedPlan = InAppPurchaseWrapper.SubscriptionType.YEARLY;
                    updateSelectedPlanView();
                });
        mUpgradeButton.setOnClickListener(
                v -> {
                    ProductDetails productDetails = null;
                    if (mCurrentSelectedPlan == InAppPurchaseWrapper.SubscriptionType.MONTHLY) {
                        productDetails = mMonthlyProductDetails;
                    } else if (mCurrentSelectedPlan
                            == InAppPurchaseWrapper.SubscriptionType.YEARLY) {
                        productDetails = mYearlyProductDetails;
                    }
                    if (productDetails == null) {
                        return;
                    }
                    InAppPurchaseWrapper.getInstance()
                            .initiatePurchase(BraveLeoPlansActivity.this, productDetails);
                });

        TextView refreshCredentialsButton = findViewById(R.id.refresh_credentials_button);
        refreshCredentialsButton.setOnClickListener(
                v -> {
                    TabUtils.openURLWithBraveActivity(
                            LinkSubscriptionUtils.getBraveAccountRecoverUrl(
                                    InAppPurchaseWrapper.SubscriptionProduct.LEO));
                });

        onInitialLayoutInflationComplete();
    }

    private void updateSelectedPlanView() {
        mYearlySelectorLayout.setBackgroundResource(
                mCurrentSelectedPlan == InAppPurchaseWrapper.SubscriptionType.YEARLY
                        ? R.drawable.leo_plan_active_bg
                        : R.drawable.leo_plan_non_active_bg);
        mMonthlySelectorLayout.setBackgroundResource(
                mCurrentSelectedPlan == InAppPurchaseWrapper.SubscriptionType.MONTHLY
                        ? R.drawable.leo_plan_active_bg
                        : R.drawable.leo_plan_non_active_bg);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        getMonthlyProductDetails();
        InAppPurchaseWrapper.getInstance()
                .queryProductDetailsAsync(InAppPurchaseWrapper.SubscriptionProduct.LEO);
    }

    private void getYearlyProductDetails() {
        mYearlyPlanProgress.setVisibility(View.VISIBLE);
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getYearlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.LEO),
                yearlyProductDetails -> {
                    mYearlyProductDetails = yearlyProductDetails;
                    workWithYearlyPurchase();
                });
    }

    private void getMonthlyProductDetails() {
        mMonthlyPlanProgress.setVisibility(View.VISIBLE);
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getMonthlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.LEO),
                monthlyProductDetails -> {
                    mMonthlyProductDetails = monthlyProductDetails;
                    workWithMonthlyPurchase();
                    getYearlyProductDetails();
                });
    }

    private void workWithYearlyPurchase() {
        if (mYearlyProductDetails == null) {
            return;
        }
        runOnUiThread(
                new Runnable() {
                    @Override
                    public void run() {
                        SpannableString yearlyFormattedPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedProductPrice(
                                                BraveLeoPlansActivity.this,
                                                mYearlyProductDetails,
                                                R.string.yearly_subscription_amount);
                        if (yearlyFormattedPrice != null) {
                            mYearlySubscriptionAmountText.setText(
                                    yearlyFormattedPrice, TextView.BufferType.SPANNABLE);
                            mYearlyPlanProgress.setVisibility(View.GONE);
                        }
                    }
                });
    }

    private void workWithMonthlyPurchase() {
        if (mMonthlyProductDetails == null) {
            return;
        }
        runOnUiThread(
                new Runnable() {
                    @Override
                    public void run() {
                        SpannableString monthlyFormattedPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedProductPrice(
                                                BraveLeoPlansActivity.this,
                                                mMonthlyProductDetails,
                                                R.string.monthly_subscription_amount);
                        if (monthlyFormattedPrice != null) {
                            mMonthlySubscriptionAmountText.setText(
                                    monthlyFormattedPrice, TextView.BufferType.SPANNABLE);
                            mMonthlyPlanProgress.setVisibility(View.GONE);
                        }
                    }
                });
    }

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}

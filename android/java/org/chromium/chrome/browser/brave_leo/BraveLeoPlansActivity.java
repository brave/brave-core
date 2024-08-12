/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.text.SpannableString;
import android.view.View;
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
    private TextView mMonthlySubscriptionAmountText;
    private TextView mUpgradeButton;

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

        TextView refreshCredentialsButton = findViewById(R.id.refresh_credentials_button);
        refreshCredentialsButton.setOnClickListener(
                v -> {
                    TabUtils.openURLWithBraveActivity(
                            LinkSubscriptionUtils.getBraveAccountRecoverUrl(
                                    InAppPurchaseWrapper.SubscriptionProduct.LEO));
                });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mMonthlyPlanProgress.setVisibility(View.VISIBLE);
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getMonthlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.LEO),
                monthlyProductDetails -> {
                    workWithMonthlyPurchase(monthlyProductDetails);
                });
        InAppPurchaseWrapper.getInstance()
                .queryProductDetailsAsync(InAppPurchaseWrapper.SubscriptionProduct.LEO);
    }

    private void workWithMonthlyPurchase(ProductDetails monthlyProductDetails) {
        if (monthlyProductDetails == null) {
            return;
        }
        runOnUiThread(
                new Runnable() {
                    @Override
                    public void run() {
                        mUpgradeButton.setOnClickListener(
                                v ->
                                        InAppPurchaseWrapper.getInstance()
                                                .initiatePurchase(
                                                        BraveLeoPlansActivity.this,
                                                        monthlyProductDetails));
                        SpannableString monthlyFormattedPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedProductPrice(
                                                BraveLeoPlansActivity.this,
                                                monthlyProductDetails,
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

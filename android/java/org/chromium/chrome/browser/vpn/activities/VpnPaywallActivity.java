/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.activities;

import static android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE;

import android.graphics.Paint;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.text.style.TextAppearanceSpan;
import android.text.style.UnderlineSpan;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import com.android.billingclient.api.ProductDetails;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

public class VpnPaywallActivity extends BraveVpnParentActivity {
    private LinearLayout mPlanLayout;
    private boolean mShouldShowRestoreMenu;

    private ProgressBar mMonthlyPlanProgress;
    private LinearLayout mMonthlySelectorLayout;
    private TextView mMonthlySubscriptionAmountText;

    private ProgressBar mYearlyPlanProgress;
    private LinearLayout mYearlySelectorLayout;
    private TextView mYearlySubscriptionAmountText;
    private TextView mRemovedValueText;
    private TextView mYearlyText;

    private Button mBtnVpnPlanAction;

    private InAppPurchaseWrapper.SubscriptionType mSelectedPlanType =
            InAppPurchaseWrapper.SubscriptionType.YEARLY;

    private ProductDetails mMonthlyProductDetails;
    private ProductDetails mYearlyProductDetails;

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveVpnUtils.dismissProgressDialog();
    }

    private void initializeViews() {
        setContentView(R.layout.activity_vpn_paywall);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.brave_vpn));

        mPlanLayout = findViewById(R.id.plan_layout);

        mMonthlyPlanProgress = findViewById(R.id.monthly_plan_progress);

        mRemovedValueText = findViewById(R.id.removed_value_tv);
        mRemovedValueText.setPaintFlags(
                mRemovedValueText.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);

        TextView monthlySubscriptionText = findViewById(R.id.monthly_subscription_text);
        monthlySubscriptionText.setText(
                String.format(getResources().getString(R.string.monthly_subscription), ""));
        mMonthlySubscriptionAmountText = findViewById(R.id.monthly_subscription_amount_text);
        mMonthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        mMonthlySelectorLayout.setOnClickListener(
                v -> {
                    mSelectedPlanType = InAppPurchaseWrapper.SubscriptionType.MONTHLY;
                    updateSelectedPlanView();
                });

        TextView trialText = findViewById(R.id.trial_text);
        String trialString = getResources().getString(R.string.trial_text);
        String day7TrialString = getResources().getString(R.string.day_7_trial_text);
        String trialFullString = String.format(trialString, day7TrialString);
        SpannableString trialTextSpan = new SpannableString(trialFullString);
        int trialTextIndex = trialFullString.indexOf(day7TrialString);
        trialTextSpan.setSpan(
                new UnderlineSpan(),
                trialTextIndex,
                trialFullString.length(),
                SPAN_EXCLUSIVE_EXCLUSIVE);
        trialTextSpan.setSpan(
                new TextAppearanceSpan(VpnPaywallActivity.this, R.style.DefaultSemibold),
                trialTextIndex,
                trialFullString.length(),
                SPAN_EXCLUSIVE_EXCLUSIVE);
        trialText.setText(trialTextSpan);

        mYearlyPlanProgress = findViewById(R.id.yearly_plan_progress);
        mYearlySubscriptionAmountText = findViewById(R.id.yearly_subscription_amount_text);
        mYearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        mYearlySelectorLayout.setOnClickListener(
                v -> {
                    mSelectedPlanType = InAppPurchaseWrapper.SubscriptionType.YEARLY;
                    updateSelectedPlanView();
                });

        mYearlyText = findViewById(R.id.yearly_text);

        TextView refreshCredentialsButton = findViewById(R.id.refresh_credentials_button);
        refreshCredentialsButton.setOnClickListener(
                v -> {
                    TabUtils.openURLWithBraveActivity(
                            LinkSubscriptionUtils.getBraveAccountRecoverUrl(
                                    InAppPurchaseWrapper.SubscriptionProduct.VPN));
                });

        mBtnVpnPlanAction = findViewById(R.id.vpn_plan_action_button);
        mBtnVpnPlanAction.setOnClickListener(
                v -> {
                    ProductDetails productDetails = null;
                    if (mSelectedPlanType == InAppPurchaseWrapper.SubscriptionType.MONTHLY) {
                        productDetails = mMonthlyProductDetails;
                    } else if (mSelectedPlanType == InAppPurchaseWrapper.SubscriptionType.YEARLY) {
                        productDetails = mYearlyProductDetails;
                    }
                    if (productDetails == null) {
                        return;
                    }
                    InAppPurchaseWrapper.getInstance()
                            .initiatePurchase(VpnPaywallActivity.this, productDetails);
                });
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        // Check for an active subscription to show restore
        BraveVpnUtils.showProgressDialog(
                VpnPaywallActivity.this, getResources().getString(R.string.vpn_connect_text));
        mIsVerification = true;
        verifySubscription();

        getMonthlyProductDetails();
        updateSelectedPlanView();
    }

    private void getMonthlyProductDetails() {
        // Set up monthly subscription
        mMonthlyPlanProgress.setVisibility(View.VISIBLE);
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getMonthlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.VPN),
                monthlyProductDetails -> {
                    mMonthlyProductDetails = monthlyProductDetails;
                    updateMonthlyPlanUI();
                    getYearlyProductDetails();
                });
    }

    private void getYearlyProductDetails() {
        mYearlyPlanProgress.setVisibility(View.VISIBLE);
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getYearlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.VPN),
                yearlyProductDetails -> {
                    mYearlyProductDetails = yearlyProductDetails;
                    updateYearlyPlanUI();
                });
    }

    private void updateSelectedPlanView() {
        mYearlySelectorLayout.setBackgroundResource(
                mSelectedPlanType == InAppPurchaseWrapper.SubscriptionType.YEARLY
                        ? R.drawable.vpn_paywall_selected_bg
                        : R.drawable.vpn_paywall_bg);
        mMonthlySelectorLayout.setBackgroundResource(
                mSelectedPlanType == InAppPurchaseWrapper.SubscriptionType.MONTHLY
                        ? R.drawable.vpn_paywall_selected_bg
                        : R.drawable.vpn_paywall_bg);
    }

    private void updateMonthlyPlanUI() {
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
                                                VpnPaywallActivity.this,
                                                mMonthlyProductDetails,
                                                R.string.monthly_subscription_amount);
                        if (monthlyFormattedPrice != null) {
                            mMonthlySubscriptionAmountText.setText(
                                    monthlyFormattedPrice, TextView.BufferType.SPANNABLE);
                            mMonthlyPlanProgress.setVisibility(View.GONE);
                        }
                        SpannableString fullPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedFullProductPrice(
                                                VpnPaywallActivity.this, mMonthlyProductDetails);
                        if (fullPrice != null) {
                            mRemovedValueText.setText(fullPrice, TextView.BufferType.SPANNABLE);
                        }
                    }
                });
    }

    private void updateYearlyPlanUI() {
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
                                                VpnPaywallActivity.this,
                                                mYearlyProductDetails,
                                                R.string.yearly_subscription_amount);
                        if (yearlyFormattedPrice != null) {
                            mYearlySubscriptionAmountText.setText(
                                    yearlyFormattedPrice, TextView.BufferType.SPANNABLE);
                            mYearlyPlanProgress.setVisibility(View.GONE);
                        }
                        mBtnVpnPlanAction.setEnabled(true);
                        if (mMonthlyProductDetails != null) {
                            String annualSaveText =
                                    getString(
                                            R.string.renew_yearly_save,
                                            InAppPurchaseWrapper.getInstance()
                                                    .getYearlyDiscountPercentage(
                                                            mMonthlyProductDetails,
                                                            mYearlyProductDetails));
                            String discountText =
                                    getString(R.string.renews_annually)
                                            .concat(" ")
                                            .concat(annualSaveText);
                            SpannableString discountSpannableString =
                                    SpanApplier.applySpans(
                                            discountText,
                                            new SpanInfo(
                                                    "<discount_text>",
                                                    "</discount_text>",
                                                    new StyleSpan(android.graphics.Typeface.BOLD),
                                                    new UnderlineSpan()));
                            mYearlyText.setText(discountSpannableString);
                        }
                    }
                });
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

            if (mBtnVpnPlanAction != null) {
                mBtnVpnPlanAction.setAlpha(0.4f);
                mBtnVpnPlanAction.setOnClickListener(null);
            }
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        } else if (item.getItemId() == R.id.restore) {
            BraveVpnUtils.openBraveVpnProfileActivity(VpnPaywallActivity.this);
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void triggerLayoutInflation() {
        initializeViews();
        onInitialLayoutInflationComplete();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void showRestoreMenu(boolean shouldShowRestore) {
        this.mShouldShowRestoreMenu = shouldShowRestore;
        InAppPurchaseWrapper.getInstance()
                .queryProductDetailsAsync(InAppPurchaseWrapper.SubscriptionProduct.VPN);
        invalidateOptionsMenu();
    }

    @Override
    public void updateProfileView() {}
}

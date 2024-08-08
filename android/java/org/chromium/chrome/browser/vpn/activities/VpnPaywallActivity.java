/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.activities;

import static android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE;

import android.graphics.Paint;
import android.text.SpannableString;
import android.text.style.StyleSpan;
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
import androidx.viewpager.widget.ViewPager;

import com.android.billingclient.api.ProductDetails;

import com.google.android.material.tabs.TabLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.adapters.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

public class VpnPaywallActivity extends BraveVpnParentActivity {
    private ProgressBar mMonthlyPlanProgress;
    private ProgressBar mYearlyPlanProgress;
    private LinearLayout mPlanLayout;
    private boolean mShouldShowRestoreMenu;

    private LinearLayout mMonthlySelectorLayout;
    private TextView mMonthlySubscriptionAmountText;

    private LinearLayout mYearlySelectorLayout;
    private TextView mYearlySubscriptionAmountText;
    private TextView mRemovedValueText;
    private TextView mYearlyText;

    private Button mBtnVpnPlanAction;

    enum SelectedPlanType {
        YEARLY,
        MONTHLY
    }

    private SelectedPlanType mSelectedPlanType = SelectedPlanType.YEARLY;
    private ProductDetails mProductDetails;

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveVpnNativeWorker.getInstance().addObserver(this);
        BraveVpnUtils.dismissProgressDialog();
    }

    @Override
    public void onPauseWithNative() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onPauseWithNative();
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

        mMonthlyPlanProgress = findViewById(R.id.monthly_plan_progress);

        mYearlyPlanProgress = findViewById(R.id.yearly_plan_progress);
        mPlanLayout = findViewById(R.id.plan_layout);

        mRemovedValueText = findViewById(R.id.removed_value_tv);
        mRemovedValueText.setPaintFlags(
                mRemovedValueText.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);

        TextView monthlySubscriptionText = findViewById(R.id.monthly_subscription_text);
        monthlySubscriptionText.setText(
                String.format(getResources().getString(R.string.monthly_subscription), ""));
        mMonthlySubscriptionAmountText = findViewById(R.id.monthly_subscription_amount_text);
        mMonthlySelectorLayout = findViewById(R.id.monthly_selector_layout);

        TextView trialText = findViewById(R.id.trial_text);
        String trialString = getResources().getString(R.string.trial_text);
        String day7TrialString = getResources().getString(R.string.day_7_trial_text);
        String trialFullString = String.format(trialString, day7TrialString);
        SpannableString trialTextSpan = new SpannableString(trialFullString);
        int trialTextIndex = trialFullString.indexOf(day7TrialString);
        trialTextSpan.setSpan(
                new android.text.style.StyleSpan(android.graphics.Typeface.BOLD),
                trialTextIndex,
                trialFullString.length(),
                SPAN_EXCLUSIVE_EXCLUSIVE);
        trialTextSpan.setSpan(
                new UnderlineSpan(),
                trialTextIndex,
                trialFullString.length(),
                SPAN_EXCLUSIVE_EXCLUSIVE);
        trialText.setText(trialTextSpan);

        mYearlySubscriptionAmountText = findViewById(R.id.yearly_subscription_amount_text);
        mYearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
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
                    if (mProductDetails != null) {
                        InAppPurchaseWrapper.getInstance()
                                .initiatePurchase(VpnPaywallActivity.this, mProductDetails);
                    }
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
                    if (monthlyProductDetails != null) {
                        runOnUiThread(
                                new Runnable() {
                                    @Override
                                    public void run() {
                                        mMonthlySelectorLayout.setOnClickListener(
                                                v -> {
                                                    mSelectedPlanType = SelectedPlanType.MONTHLY;
                                                    mProductDetails = monthlyProductDetails;
                                                    updateSelectedPlanView();
                                                });
                                        String monthlyFormattedPrice =
                                                InAppPurchaseWrapper.getInstance()
                                                        .getFormattedProductPrice(
                                                                monthlyProductDetails);
                                        if (monthlyFormattedPrice != null) {
                                            mMonthlySubscriptionAmountText.setText(
                                                    String.format(
                                                            getResources()
                                                                    .getString(
                                                                            R.string
                                                                                    .monthly_subscription_amount),
                                                            monthlyFormattedPrice));
                                            mMonthlyPlanProgress.setVisibility(View.GONE);
                                        }
                                        String fullPrice =
                                                InAppPurchaseWrapper.getInstance()
                                                        .getFormattedFullProductPrice(
                                                                monthlyProductDetails);
                                        if (fullPrice != null) {
                                            mRemovedValueText.setText(fullPrice);
                                        }
                                    }
                                });
                        getYearlyProductDetails(monthlyProductDetails);
                    }
                });
    }

    private void getYearlyProductDetails(ProductDetails monthlyProductDetails) {
        mYearlyPlanProgress.setVisibility(View.VISIBLE);
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance().getYearlyProductDetails(),
                yearlyProductDetails -> {
                    if (yearlyProductDetails != null) {
                        runOnUiThread(
                                new Runnable() {
                                    @Override
                                    public void run() {
                                        mYearlySelectorLayout.setOnClickListener(
                                                v -> {
                                                    mSelectedPlanType = SelectedPlanType.YEARLY;
                                                    mProductDetails = yearlyProductDetails;
                                                    updateSelectedPlanView();
                                                });
                                        String yearlyFormattedPrice =
                                                InAppPurchaseWrapper.getInstance()
                                                        .getFormattedProductPrice(
                                                                yearlyProductDetails);
                                        if (yearlyFormattedPrice != null) {
                                            mYearlySubscriptionAmountText.setText(
                                                    String.format(
                                                            getResources()
                                                                    .getString(
                                                                            R.string
                                                                                    .yearly_subscription_amount),
                                                            yearlyFormattedPrice));
                                            mYearlyPlanProgress.setVisibility(View.GONE);
                                        }
                                    }
                                });
                        mProductDetails = yearlyProductDetails;
                        mBtnVpnPlanAction.setEnabled(true);
                        if (monthlyProductDetails != null) {
                            String discountText =
                                    getString(
                                            R.string.renew_monthly_save,
                                            InAppPurchaseWrapper.getInstance()
                                                    .getYearlyDiscountPercentage(
                                                            monthlyProductDetails,
                                                            yearlyProductDetails));
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

    private void updateSelectedPlanView() {
        mYearlySelectorLayout.setBackgroundResource(
                mSelectedPlanType == SelectedPlanType.YEARLY
                        ? R.drawable.vpn_plan_selected_bg
                        : R.drawable.vpn_plan_bg);
        mMonthlySelectorLayout.setBackgroundResource(
                mSelectedPlanType == SelectedPlanType.MONTHLY
                        ? R.drawable.vpn_plan_selected_bg
                        : R.drawable.vpn_plan_bg);
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

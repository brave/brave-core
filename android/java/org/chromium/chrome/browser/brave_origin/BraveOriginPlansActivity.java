/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import android.graphics.Paint;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.text.style.UnderlineSpan;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import com.android.billingclient.api.ProductDetails;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.SpanApplier;

/**
 * Activity for displaying Brave Origin subscription plans and handling purchases.
 *
 * <p>This activity presents users with subscription options for Brave Origin premium features,
 * including enhanced privacy protection, advanced features, premium support, and exclusive content.
 * It integrates with Google Play Billing to handle in-app purchases and subscription management.
 *
 * <p>This activity: - Displays monthly and yearly subscription options - Shows premium feature
 * benefits - Handles subscription purchase flow - Manages existing subscription credentials
 *
 * <p>The activity extends AsyncInitializationActivity to ensure proper Chrome initialization before
 * displaying the UI.
 */
public class BraveOriginPlansActivity extends AsyncInitializationActivity {
    // UI components for displaying subscription plan progress indicators
    private ProgressBar mMonthlyPlanProgress;
    private ProgressBar mYearlyPlanProgress;

    // Text views for displaying formatted subscription prices
    private TextView mMonthlySubscriptionAmountText;
    private TextView mYearlySubscriptionAmountText;

    // Upgrade button for initiating subscription purchase
    private TextView mUpgradeButton;
    private TextView mYearlyText;
    private TextView mRemovedValueText;

    // Currently selected subscription plan (defaults to yearly for better value)
    private InAppPurchaseWrapper.SubscriptionType mCurrentSelectedPlan =
            InAppPurchaseWrapper.SubscriptionType.YEARLY;

    // Layout containers for subscription plan selection
    private LinearLayout mMonthlySelectorLayout;
    private LinearLayout mYearlySelectorLayout;

    // Google Play Billing product details for each subscription type
    ProductDetails mMonthlyProductDetails;
    ProductDetails mYearlyProductDetails;

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    /** Initializes the UI layout and sets up event handlers. */
    @Override
    protected void triggerLayoutInflation() {
        // Set the main layout for the Origin plans activity
        setContentView(R.layout.activity_brave_origin_plans);

        // Configure the toolbar with close button and title
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.brave_origin_premium));

        // This section contains code to set up subscription plan selectors,
        // upgrade button click handlers, and refresh credentials functionality
        mMonthlyPlanProgress = findViewById(R.id.monthly_plan_progress);
        mMonthlySubscriptionAmountText = findViewById(R.id.monthly_subscription_amount_text);
        mUpgradeButton = findViewById(R.id.tv_upgrade_now);
        mYearlyPlanProgress = findViewById(R.id.yearly_plan_progress);
        mYearlySubscriptionAmountText = findViewById(R.id.yearly_subscription_amount_text);
        mMonthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        mYearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        mYearlyText = findViewById(R.id.yearly_text);
        mRemovedValueText = findViewById(R.id.removed_value_tv);
        mRemovedValueText.setPaintFlags(
                mRemovedValueText.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
        // Set up click listeners for subscription plan selectors
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
        // Set up click listener for upgrade button
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
                    // Always use the regular profile as this activity is only launched from
                    // Settings
                    Profile currentProfile =
                            getProfileProviderSupplier().get().getOriginalProfile();
                    InAppPurchaseWrapper.getInstance()
                            .initiatePurchase(
                                    BraveOriginPlansActivity.this, productDetails, currentProfile);
                });

        TextView refreshCredentialsButton = findViewById(R.id.refresh_credentials_button);
        // Set up click listener for refresh credentials button
        refreshCredentialsButton.setOnClickListener(
                v -> {
                    // Open Brave account recovery URL to refresh credentials
                    TabUtils.openURLWithBraveActivity(
                            LinkSubscriptionUtils.getBraveAccountRecoverUrl(
                                    InAppPurchaseWrapper.SubscriptionProduct.ORIGIN));
                });

        onInitialLayoutInflationComplete();
    }

    /**
     * Updates the visual appearance of subscription plan selectors based on current selection.
     * Changes the background drawable to highlight the selected plan and dim the unselected one.
     */
    private void updateSelectedPlanView() {
        // Update yearly plan selector background based on selection state
        mYearlySelectorLayout.setBackgroundResource(
                mCurrentSelectedPlan == InAppPurchaseWrapper.SubscriptionType.YEARLY
                        ? R.drawable
                                .origin_plan_active_bg // Highlighted background for selected plan
                        : R.drawable.origin_plan_non_active_bg); // Dimmed background for unselected
        // plan

        // Update monthly plan selector background based on selection state
        mMonthlySelectorLayout.setBackgroundResource(
                mCurrentSelectedPlan == InAppPurchaseWrapper.SubscriptionType.MONTHLY
                        ? R.drawable
                                .origin_plan_active_bg // Highlighted background for selected plan
                        : R.drawable.origin_plan_non_active_bg); // Dimmed background for unselected
        // plan
    }

    /**
     * Called when native Chrome initialization is complete. This is the appropriate time to start
     * billing operations and fetch product details.
     */
    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        getMonthlyProductDetails();
        InAppPurchaseWrapper.getInstance()
                .queryProductDetailsAsync(InAppPurchaseWrapper.SubscriptionProduct.ORIGIN);
    }

    /**
     * Fetches yearly subscription product details from Google Play Billing. Shows a progress
     * indicator while loading and processes the result when available.
     */
    private void getYearlyProductDetails() {
        // Show progress indicator while fetching product details
        mYearlyPlanProgress.setVisibility(View.VISIBLE);

        // Observe the yearly product details LiveData for Origin subscription
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getYearlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.ORIGIN),
                yearlyProductDetails -> {
                    // Store the product details for later use in purchase flow
                    mYearlyProductDetails = yearlyProductDetails;
                    // Format and display the yearly subscription price
                    workWithYearlyPurchase();
                });
    }

    /**
     * Fetches monthly subscription product details from Google Play Billing. Shows a progress
     * indicator while loading, processes the result, and then fetches yearly details.
     */
    private void getMonthlyProductDetails() {
        // Show progress indicator while fetching product details
        mMonthlyPlanProgress.setVisibility(View.VISIBLE);

        // Observe the monthly product details LiveData for Origin subscription
        LiveDataUtil.observeOnce(
                InAppPurchaseWrapper.getInstance()
                        .getMonthlyProductDetails(InAppPurchaseWrapper.SubscriptionProduct.ORIGIN),
                monthlyProductDetails -> {
                    // Store the product details for later use in purchase flow
                    mMonthlyProductDetails = monthlyProductDetails;
                    // Format and display the monthly subscription price
                    workWithMonthlyPurchase();
                    // Fetch yearly details after monthly details are loaded
                    getYearlyProductDetails();
                });
    }

    /**
     * Processes yearly subscription product details and updates the UI with formatted pricing. This
     * method formats the price according to user's locale and currency, then displays it.
     */
    private void workWithYearlyPurchase() {
        if (mYearlyProductDetails == null) {
            return; // Exit early if no product details available
        }

        // Update UI on main thread since this might be called from a background thread
        runOnUiThread(
                new Runnable() {
                    @Override
                    public void run() {
                        // Format the yearly subscription price using the billing wrapper
                        // This handles currency conversion and locale-specific formatting
                        SpannableString yearlyFormattedPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedProductPrice(
                                                BraveOriginPlansActivity.this,
                                                mYearlyProductDetails,
                                                R.string.yearly_subscription_amount_origin);

                        if (yearlyFormattedPrice != null) {
                            // Display the formatted price and hide the progress indicator
                            mYearlySubscriptionAmountText.setText(
                                    yearlyFormattedPrice, TextView.BufferType.SPANNABLE);
                            mYearlyPlanProgress.setVisibility(View.GONE);
                        }
                        // Display the annual save text
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
                            // Apply bold and underline spans to the discount text
                            SpannableString discountSpannableString =
                                    SpanApplier.applySpans(
                                            discountText,
                                            new SpanApplier.SpanInfo(
                                                    "<discount_text>",
                                                    "</discount_text>",
                                                    new StyleSpan(android.graphics.Typeface.BOLD),
                                                    new UnderlineSpan()));
                            mYearlyText.setText(discountSpannableString);
                        }
                    }
                });
    }

    /**
     * Processes monthly subscription product details and updates the UI with formatted pricing.
     * This method formats the price according to user's locale and currency, then displays it.
     */
    private void workWithMonthlyPurchase() {
        if (mMonthlyProductDetails == null) {
            return; // Exit early if no product details available
        }

        // Update UI on main thread since this might be called from a background thread
        runOnUiThread(
                new Runnable() {
                    @Override
                    public void run() {
                        // Format the monthly subscription price using the billing wrapper
                        // This handles currency conversion and locale-specific formatting
                        SpannableString monthlyFormattedPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedProductPrice(
                                                BraveOriginPlansActivity.this,
                                                mMonthlyProductDetails,
                                                R.string.monthly_subscription_amount_origin);

                        if (monthlyFormattedPrice != null) {
                            // Display the formatted price and hide the progress indicator
                            mMonthlySubscriptionAmountText.setText(
                                    monthlyFormattedPrice, TextView.BufferType.SPANNABLE);
                            mMonthlyPlanProgress.setVisibility(View.GONE);
                        }
                        // Display the full price with strike-through
                        SpannableString fullPrice =
                                InAppPurchaseWrapper.getInstance()
                                        .getFormattedFullProductPrice(
                                                BraveOriginPlansActivity.this,
                                                mMonthlyProductDetails);
                        if (fullPrice != null) {
                            mRemovedValueText.setText(fullPrice, TextView.BufferType.SPANNABLE);
                        }
                    }
                });
    }

    @NonNull
    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.TabUtils;

// TODO: Uncomment when hooking up one-time purchase billing flow
// import android.view.View;
// import android.widget.ProgressBar;
// import com.android.billingclient.api.ProductDetails;
// import org.chromium.chrome.browser.profiles.Profile;
// import org.chromium.chrome.browser.util.LiveDataUtil;

/**
 * Activity for displaying Brave Origin one-time purchase plan and handling purchases.
 *
 * <p>This activity presents users with a one-time purchase option for Brave Origin, which provides
 * a minimalist browser UI centered on Brave Shields with core adblock, privacy, and speed. It
 * integrates with Google Play Billing to handle in-app purchases.
 *
 * <p>The activity extends AsyncInitializationActivity to ensure proper Chrome initialization before
 * displaying the UI.
 */
public class BraveOriginPlansActivity extends AsyncInitializationActivity {
    // TODO: Hook up one-time purchase billing flow
    // private ProgressBar mPurchasePlanProgress;
    // private TextView mPurchaseAmountText;
    private TextView mBuyNowButton;

    // ProductDetails mProductDetails;

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    /** Initializes the UI layout and sets up event handlers. */
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_brave_origin_plans);

        // Configure the toolbar with close button and title
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.brave_origin_premium));

        // TODO: Hook up one-time purchase billing flow
        // mPurchasePlanProgress = findViewById(R.id.purchase_plan_progress);
        // mPurchaseAmountText = findViewById(R.id.purchase_amount_text);
        mBuyNowButton = findViewById(R.id.tv_buy_now);

        // TODO: Hook up one-time purchase billing flow
        mBuyNowButton.setOnClickListener(
                v -> {
                    // if (mProductDetails == null) {
                    //     return;
                    // }
                    // Profile currentProfile =
                    //         getProfileProviderSupplier().get().getOriginalProfile();
                    // InAppPurchaseWrapper.getInstance()
                    //         .initiatePurchase(BraveOriginPlansActivity.this,
                    //                 mProductDetails, currentProfile);
                });

        TextView getLoginCodeButton = findViewById(R.id.get_login_code_button);
        // Set up click listener for get login code button
        getLoginCodeButton.setOnClickListener(
                v -> {
                    TabUtils.openURLWithBraveActivity(
                            LinkSubscriptionUtils.getBraveAccountRecoverUrl(
                                    InAppPurchaseWrapper.SubscriptionProduct.ORIGIN));
                });

        onInitialLayoutInflationComplete();
    }

    /**
     * Called when native Chrome initialization is complete. This is the appropriate time to start
     * billing operations and fetch product details.
     */
    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        // TODO: Hook up one-time purchase product details query
        // getProductDetails();
        // InAppPurchaseWrapper.getInstance()
        //         .queryProductDetailsAsync(InAppPurchaseWrapper.SubscriptionProduct.ORIGIN);
    }

    /**
     * Fetches one-time purchase product details from Google Play Billing. Shows a progress
     * indicator while loading and processes the result when available.
     */
    // TODO: Hook up one-time purchase product details fetching
    // private void getProductDetails() {
    //     mPurchasePlanProgress.setVisibility(View.VISIBLE);
    //
    //     LiveDataUtil.observeOnce(
    //             InAppPurchaseWrapper.getInstance()
    //                     .getOriginProductDetails(),
    //             productDetails -> {
    //                 mProductDetails = productDetails;
    //                 displayPrice();
    //             });
    // }

    // TODO: Hook up one-time purchase price display
    // private void displayPrice() {
    //     if (mProductDetails == null) {
    //         return;
    //     }
    //
    //     runOnUiThread(
    //             () -> {
    //                 String formattedPrice =
    //                         InAppPurchaseWrapper.getInstance()
    //                                 .getFormattedOneTimePurchasePrice(mProductDetails);
    //
    //                 if (formattedPrice != null) {
    //                     mPurchaseAmountText.setText(formattedPrice);
    //                     mPurchasePlanProgress.setVisibility(View.GONE);
    //                 }
    //             });
    // }

    @NonNull
    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}

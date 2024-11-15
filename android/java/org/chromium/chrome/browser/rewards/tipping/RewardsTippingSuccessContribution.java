/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.view.View;

import androidx.annotation.NonNull;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.ui.base.DeviceFormFactor;

public class RewardsTippingSuccessContribution {
    private static final String TAG = "TippingSuccess";
    private View mContentView;
    private double mAmountSelected;
    private Activity mActivity;
    private boolean mIsTablet;

    public RewardsTippingSuccessContribution(
            @NonNull Activity activity, View rootView, double amountSelected) {
        mContentView = rootView;
        mAmountSelected = amountSelected;
        mActivity = activity;
        setShareYourSupportClickListener();
        setGoBackClick();
        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);
    }

    private void setGoBackClick() {
        View goBackButton = mContentView.findViewById(R.id.go_back_button);
        goBackButton.setOnClickListener(v -> { dismissRewardsPanel(); });
    }

    private void dismissRewardsPanel() {
        if (mIsTablet) {
            try {
                BraveActivity braveActivity = BraveActivity.getBraveActivity();
                braveActivity.dismissRewardsPanel();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "setShareYourSupportClickListener " + e);
            }
        } else {
            mActivity.setResult(Activity.RESULT_OK);
            mActivity.finish();
        }
    }

    private void setShareYourSupportClickListener() {
        View shareYourSupportButton = mContentView.findViewById(R.id.share_your_support);
        shareYourSupportButton.setOnClickListener(v -> {
            double amount = mAmountSelected;

            String tweetUrl = getTipSuccessTweetUrl(amount);
            Uri uri = Uri.parse(tweetUrl);
            mActivity.startActivity(new Intent(Intent.ACTION_VIEW, uri));
            dismissRewardsPanel();
        });
    }

    private String getTipSuccessTweetUrl(double amount) {
        Uri.Builder builder = new Uri.Builder();
        builder.scheme("https")
                .authority("twitter.com")
                .appendPath("intent")
                .appendPath("tweet")
                .appendQueryParameter("text",
                        String.format(mActivity.getString(R.string.brave_rewards_tip_success_tweet),
                                amount));
        return builder.build().toString();
    }
}

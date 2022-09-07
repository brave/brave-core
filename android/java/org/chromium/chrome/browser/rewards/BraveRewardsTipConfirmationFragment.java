
/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;

public class BraveRewardsTipConfirmationFragment extends Fragment {
    public static BraveRewardsTipConfirmationFragment newInstance(
            int status, String name, double amount, boolean isMonthly) {
        BraveRewardsTipConfirmationFragment fragment = new BraveRewardsTipConfirmationFragment();
        Bundle args = new Bundle();
        args.putDouble(BraveRewardsSiteBannerActivity.AMOUNT_EXTRA, amount);
        args.putBoolean(BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, isMonthly);
        args.putInt(BraveRewardsSiteBannerActivity.STATUS_ARGS, status);
        args.putString(BraveRewardsSiteBannerActivity.NAME_ARGS, name);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.brave_rewards_tip_confirmation, container, false);
        if (getArguments() != null) {
            double amount = getArguments().getDouble(BraveRewardsSiteBannerActivity.AMOUNT_EXTRA);
            boolean monthlyContribution = getArguments().getBoolean(
                    BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION);
            int status = getArguments().getInt(BraveRewardsSiteBannerActivity.STATUS_ARGS, -1);
            String publisherName =
                    getArguments().getString(BraveRewardsSiteBannerActivity.NAME_ARGS);

            initView(view, status, publisherName, amount, monthlyContribution);
        }
        return view;
    }

    int mStatus;
    String mPublisherName;

    @SuppressLint("SetTextI18n")
    private void initView(
            View view, int status, String name, double amount, boolean monthlyContribution) {
        mStatus = status;
        mPublisherName = name;
        TextView tipAmountTextView = view.findViewById(R.id.amount_sent);
        tipAmountTextView.setText(amount + " BAT");
        TextView tipTypeDescriptionTextView = view.findViewById(R.id.your_tip_amount_sent);

        if (monthlyContribution) {
            tipTypeDescriptionTextView.setText(
                    getResources().getString(R.string.your_monthly_tip_has_been_sent));
        } else {
            tipTypeDescriptionTextView.setText(
                    getResources().getString(R.string.your_one_time_tip_has_been_sent));
        }

        setTweetClickListener(view);
    }

    private void setTweetClickListener(View view) {
        ImageView twitter_button = view.findViewById(R.id.twitter_button);
        twitter_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String tweetUrl;
                if (mStatus == BraveRewardsSiteBannerActivity.TIP_SUCCESS) {
                    tweetUrl = getTipSuccessTweetUrl(mPublisherName);
                    Uri uri = Uri.parse(tweetUrl);
                    startActivity(new Intent(Intent.ACTION_VIEW, uri));
                } else if (mStatus == BraveRewardsSiteBannerActivity.TIP_PENDING) {
                    tweetUrl = getTipPendingTweetUrl(mPublisherName);
                    Uri uri = Uri.parse(tweetUrl);
                    startActivity(new Intent(Intent.ACTION_VIEW, uri));
                } else {
                    // error
                }
            }
        });
    }

    private String getTipPendingTweetUrl(String publisherName) {
        Calendar c = Calendar.getInstance();
        c.add(Calendar.DATE, 90);

        DateFormat dateFormat = new SimpleDateFormat("MMM dd, yyyy", Locale.getDefault());

        Uri.Builder builder = new Uri.Builder();
        builder.scheme("https")
                .authority("twitter.com")
                .appendPath("intent")
                .appendPath("tweet")
                .appendQueryParameter("text",
                        String.format(
                                getString(
                                        R.string.brave_rewards_local_compliment_tweet_unverified_publisher),
                                publisherName, dateFormat.format(c.getTime())))
                .appendQueryParameter("hashtags", "TipWithBrave");
        return builder.build().toString();
    }

    private String getTipSuccessTweetUrl(String publisherName) {
        Uri.Builder builder = new Uri.Builder();
        builder.scheme("https")
                .authority("twitter.com")
                .appendPath("intent")
                .appendPath("tweet")
                .appendQueryParameter("text",
                        String.format(getString(R.string.brave_rewards_local_compliment_tweet),
                                publisherName))
                .appendQueryParameter("hashtags", "TipWithBrave");
        return builder.build().toString();
    }
}

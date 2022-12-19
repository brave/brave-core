
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
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;

public class BraveRewardsTipConfirmationFragment extends Fragment implements BraveRewardsObserver {
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private int mStatus;
    private String mPublisherName;
    private boolean mMonthlyContribution;
    private TextView mNextTipDateText;
    private TextView mNextTipDateValue;

    private static final String TAG = "TippingBanner";

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
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);
        View view = inflater.inflate(R.layout.brave_rewards_tip_confirmation, container, false);
        if (getArguments() != null) {
            double amount = getArguments().getDouble(BraveRewardsSiteBannerActivity.AMOUNT_EXTRA);
            mMonthlyContribution = getArguments().getBoolean(
                    BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION);
            int status = getArguments().getInt(BraveRewardsSiteBannerActivity.STATUS_ARGS, -1);
            String publisherName =
                    getArguments().getString(BraveRewardsSiteBannerActivity.NAME_ARGS);

            initView(view, status, publisherName, amount, mMonthlyContribution);
        }
        return view;
    }

    @SuppressLint("SetTextI18n")
    private void initView(
            View view, int status, String name, double amount, boolean monthlyContribution) {
        mStatus = status;
        mPublisherName = name;
        TextView tipAmountTextView = view.findViewById(R.id.amount_sent);
        tipAmountTextView.setText(amount + " " + BraveRewardsHelper.BAT_TEXT);
        TextView tipTypeDescriptionTextView = view.findViewById(R.id.your_tip_amount_sent);
        TextView oneTtimeTipNote = view.findViewById(R.id.one_time_tip_note);
        String oneTimeNoteString = getResources().getString(R.string.one_time_tip_success_note);

        final StringBuilder sb1 = new StringBuilder();
        sb1.append("<b>");
        sb1.append(getResources().getString(R.string.note_text));
        sb1.append("</b>");
        sb1.append(" ");
        sb1.append(oneTimeNoteString);
        Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(sb1.toString());
        oneTtimeTipNote.setText(toInsert);

        mNextTipDateText = view.findViewById(R.id.next_tip_date_text);
        mNextTipDateValue = view.findViewById(R.id.next_tip_date_value);

        if (status == BraveRewardsSiteBannerActivity.TIP_PENDING) {
            tipTypeDescriptionTextView.setText(R.string.pending_tip_text);
            oneTtimeTipNote.setVisibility(View.INVISIBLE);
        } else if (monthlyContribution) {
            TextView tipAmountText = view.findViewById(R.id.tip_amount_text);
            tipAmountText.setText(R.string.tip_amount_text);
            tipTypeDescriptionTextView.setText(
                    getResources().getString(R.string.your_monthly_tip_has_been_sent));
        } else {
            tipTypeDescriptionTextView.setText(
                    getResources().getString(R.string.your_one_time_tip_has_been_sent));
            oneTtimeTipNote.setVisibility(View.VISIBLE);
        }
        mBraveRewardsNativeWorker.GetReconcileStamp();
        setTweetClickListener(view);
    }

    private void setTweetClickListener(View view) {
        TextView twitter_button = view.findViewById(R.id.twitter_button);
        twitter_button.setOnClickListener(v -> {
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
                Log.e(TAG,
                        "BraveRewardsTipConfirmationFragment -> setTweetClickListener invalid status ");
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

    @Override
    public void OnGetReconcileStamp(long timestamp) {
        // make reconcile date views visible
        if (true == mMonthlyContribution) {
            Date reconcileStamp = new Date(timestamp * 1000); // ms
            DateFormat formatter =
                    DateFormat.getDateInstance(DateFormat.SHORT, Locale.getDefault());

            mNextTipDateText.setVisibility(View.VISIBLE);
            mNextTipDateValue.setVisibility(View.VISIBLE);
            mNextTipDateValue.setText(formatter.format(reconcileStamp));
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }
}
